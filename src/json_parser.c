#include "ki_json/json_parser.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "print_buffer.h"
#include "ki_json/json.h"

// utf8 characters have 4 bytes max
#define CHARACTER_MAX_BUFFER_SIZE 4

//TODO: utf8 replacement char

struct json_reader
{
    const char* json_string;
    // Length of json_string (excluding null terminator)
    size_t length; 
    // Current reader index offset
    size_t offset;

    //Print buffer used for string values
    struct print_buffer string_buffer;
};

// Returns true if character is a space, horizontal tab, line feed/break or carriage return, else false.
static bool char_is_whitespace(char character)
{
    return (character == ' ') || (character == '\t') || (character == '\n') || (character == '\r');
}

#pragma region Reader

static bool reader_init(struct json_reader* reader, const char* json, size_t length)
{
    if (reader == NULL)
        return false;

    if (!print_buffer_init(&reader->string_buffer, 32))
        return false;

    reader->json_string = json;
    reader->length = length;
    reader->offset = 0;

    return true;
}

static void reader_fini(struct json_reader* reader)
{
    if (reader == NULL)
        return;

    print_buffer_fini(&reader->string_buffer);
}

// Can reader access char at index pos offsetted by the reader's offset?
static bool reader_can_access(struct json_reader* reader, size_t pos)
{
    if (reader == NULL)
        return false;
    
    return (reader->offset + pos < reader->length);
}

// Reads char in json string at index pos offsetted by the reader's offset, returns \0 on fail.
static char reader_char_at(struct json_reader* reader, size_t pos)
{
    assert(reader);

    if (!reader_can_access(reader, pos))
        return '\0';

    return reader->json_string[reader->offset + pos];
}

static bool reader_peek(struct json_reader* reader, char* character)
{
    if (reader == NULL || !reader_can_access(reader, 0))
    {
        if (character != NULL)
            *character = '\0';

        return false;
    }

    if (character != NULL)
        *character = reader->json_string[reader->offset];

    return true;
}

// Returns buffer at index pos offsetted by reader's offset, NULL on fail.
static const char* reader_buffer_at(struct json_reader* reader, size_t pos)
{
    assert(reader);

    if (!reader_can_access(reader, pos))
        return NULL;

    return reader->json_string + reader->offset + pos;
}

static void reader_skip_whitespace(struct json_reader* reader)
{
    assert(reader);

    while (reader_can_access(reader, 0) && char_is_whitespace(reader_char_at(reader, 0)))
        reader->offset++;
}

#pragma endregion

#pragma region Conversions

// Converts a hexidecimal digit to int, -1 if not a hexidecimal digit.
static int hex_digit_to_int(char hex_digit)
{
    //calculate using ASCII table

    if (hex_digit >= '0' && hex_digit <= '9')
        return (int)(hex_digit) - (int)('0');
    else if (hex_digit >= 'A' && hex_digit <= 'F')
        return (int)(hex_digit) - (int)('A') + 10;
    else if (hex_digit >= 'a' && hex_digit <= 'f')
        return (int)(hex_digit) - (int)('a') + 10;
    else
        return -1;
}

// Reads 4 hex digits in string.
// Returns read number, 0 on fail.
static uint32_t read_hex4(const char* string)
{
    assert(string);

    uint32_t num = 0;

    //read in 4 hex digits and convert them to dec
    for (int i = 0; i < 4; i++)
    {
        int digit = hex_digit_to_int(string[i]);

        //digit must be valid
        if (digit == -1)
            return 0;

        num <<= 4; //shift 4 bits to left, adding 4 zero-bits at the end
        num += (uint32_t)digit; //fill those zero-bits
    }

    return num;
}

// Convert an unicode code point to utf8 bytes.
// Returns number of bytes written, 0 on fail.
static int unicode_codepoint_to_utf8(uint32_t codepoint, unsigned char* utf8, size_t buffer_size)
{
    assert(utf8);

    //oh my god text encoding really is something, isn't it
    //SEE: https://en.wikipedia.org/wiki/UTF-8#Description

    //determine amount of utf8 bytes
    
    int bytes = 0;

    if (codepoint <= 0x007F)
        bytes = 1;
    else if (codepoint >= 0x0080 && codepoint <= 0x07FF)
        bytes = 2;
    else if (codepoint >= 0x0800 && codepoint <= 0xFFFF)
        bytes = 3;
    else if (codepoint >= 0x010000 && codepoint <= 0x10FFFF) //impossible in ki_json as only 4 hex digits are supported
        bytes = 4;

    //invalid codepoint or buffer size too small for utf8 bytes
    if (bytes == 0 || (size_t)bytes > buffer_size)
        return 0;

    if (bytes == 1)
    {
        //use 7 least significant bits of codepoint
        //keep most significant bit as 0
        utf8[0] = (unsigned char)(codepoint & ((2 << 7) - 1));
        return bytes;
    }

    //set all used bytes except first byte to begin with bits 10 = 2
    for (int i = 1; i < bytes; i++)
        utf8[i] = 2 << 6;

    //least significant bits are in last byte, so start from there
    for (int i = bytes - 1; i > 0; i--)
    {
        //get 6 least significant bits of codepoint
        unsigned char bits = (codepoint & ((2 << 6) - 1));
        //shift codepoint over by read bits
        codepoint >>= 6;
        //add bits
        utf8[i] |= bits;
    }

    //remaining bits to fill with codepoint bits
    int remaining_bits = 8 - bytes - 1;

    //first byte uses bits before first 0-bit to keep track of the amount of bytes the utf8 character uses
    //110 -> 2 bytes
    //1110 -> 3 bytes
    //11110 -> 4 bytes
    //0b10 << 2
    //0b1000 - 0b01
    //0b0111 - 0b01
    //0b0110
    utf8[0] = ((2 << bytes) - 2) << remaining_bits;

    //fill remaining bits with remaining least significant bits of codepoint
    utf8[0] |= (codepoint & ((2 << remaining_bits) - 1));

    return bytes;
}

// Escapes given char with a backslash (n -> \n, r -> \r, ...).
// Does not support \u, nor any character escape sequences that aren't used in json.
// Returns '\0' on fail.
static char char_to_single_escape_sequence_char(char type)
{
    switch(type) //escape char type
    {
        case '\"': //double quotation marks
            return '\"';
        case '\\': //reverse solidus
            return '\\'; 
        case 'b': //backspace
            return '\b';
        case 'f': //form feed
            return '\f';
        case 'n': //line feed, line break
            return '\n';
        case 'r': //carriage return
            return '\r';
        case 't': //horizontal tab
            return '\t';
        default:
            return '\0';
    }
}

#define IS_HIGH_SURROGATE(byte) (byte >= 0xD800 && byte <= 0xDBFF)
#define IS_LOW_SURROGATE(byte) (byte >= 0xDC00 && byte <= 0xDFFF)

// Converts next utf16 literal (\uXXXX or \uXXXX\uXXXX where X is any hex digit) to codepoint, outs sequence length.
// Returns codepoint, 0 on fail.
static uint32_t utf16_literal_to_codepoint(const char* literal, const char* end, size_t* sequence_length)
{
    if (literal == NULL || end == NULL)
        return 0;

    //TODO: surrogate pairs

    if (end - literal < 6)
        return 0;

    //check for \u
    if (literal[0] != '\\' || literal[1] != 'u')
        return 0;

    uint32_t codepoint = read_hex4(literal + 2);

    if (IS_HIGH_SURROGATE(codepoint))
    {
        if (sequence_length != NULL)
            *sequence_length = 12;

        //TODO: low surrogate

        if (end - literal < 12)
            return 0;

        //check for \u
        if (literal[6] != '\\' || literal[7] != 'u')
            return 0;

        uint32_t low = read_hex4(literal + 8);

        if (!IS_LOW_SURROGATE(low))
            return 0;

        //TODO: combine surrogates into single codepoint
    }
    else if (sequence_length != NULL)
    {
        *sequence_length = 6;
    }

    return codepoint;
}

// Converts next utf16 literal (\uXXXX or \uXXXX\uXXXX where X is any hex digit) to utf 8 bytes, outs sequence length.
// Returns number of bytes written, 0 on fail.
static size_t utf16_literal_to_utf8(const char* literal, const char* end, unsigned char* utf8, size_t size, size_t* sequence_length)
{
    if (literal == NULL || utf8 == NULL)
        return 0;

    uint32_t codepoint = utf16_literal_to_codepoint(literal, end, sequence_length);

    //invalid codepoint
    if (codepoint == 0)
        return 0;
    //encode codepoint as utf8 bytes into bytes buffer
    else
        return unicode_codepoint_to_utf8(codepoint, utf8, size);
}

// Converts escape sequence in (string) to utf8 bytes and output to (bytes) along with the length of read sequence.
// Supports unicode code points \uXXXX (X = hex digit), converting to utf8.
// Returns number of bytes written, 0 on fail.
static int escape_sequence_to_utf8(const char* string, const char* string_end, unsigned char* bytes, size_t buffer_size, size_t* sequence_length)
{
    assert(string && string_end && bytes && buffer_size > 0 && sequence_length);

    //backslash and escape sequence type char required
    if (string_end - string < 2)
        return 0;

    //invalid escape sequence
    if (string[0] != '\\')
        return 0;

    char escape_char_type = string[1];

    //invalid escape sequence
    if (escape_char_type == '\0')
        return 0;

    if (escape_char_type == 'u') //unicode code point, convert to utf8 bytes
    {
        size_t num_bytes = utf16_literal_to_utf8(string, string_end, bytes, buffer_size, sequence_length);

        return num_bytes;
    }
    else //single char
    {
        //buffer size must be atleast 1 byte
        if (buffer_size < 1)
            return 0;

        char escaped = char_to_single_escape_sequence_char(escape_char_type);

        //invalid type
        if (escaped == '\0')
            return 0;

        bytes[0] = (unsigned char)escaped;

        *sequence_length = 2;

        return 1;
    }
}

// Prints string inside src until end into dest, while converting escape sequences.
// Returns true on success, false on fail.
static bool print_escaped_string_as_unescaped(struct print_buffer* dest, const char* src, const char* end)
{
    if (src == NULL || end == NULL || dest == NULL)
        return false;

    //NOTE: only escaped utf8 characters are added in a single iteration, others are done byte per byte
    while (src < end)
    {
        size_t sequence_length = 0;

        //start of an escape sequence
        if (src[0] == '\\')
        {
            unsigned char bytes[CHARACTER_MAX_BUFFER_SIZE]; //character bytes, max. 4 bytes (utf8)
            size_t num_bytes = escape_sequence_to_utf8(src, end, bytes, CHARACTER_MAX_BUFFER_SIZE, &sequence_length);

            //invalid escape sequence or failed to parse it
            if (num_bytes == 0)
                return false;

            if (!print_buffer_append_mem(dest, bytes, num_bytes))
                return false;
        }
        else 
        {
            if (!print_buffer_append_char(dest, src[0]))
                return false;

            sequence_length = 1;
        }

        src += sequence_length;
    }

    return true;
}

#pragma endregion

#pragma region Parsing

// Checks whether reader can read in a string value.
// Returns true on success and outs length (including quotes), returns false on fail.
static bool has_next_string_val(struct json_reader* reader, size_t* length)
{
    if (reader == NULL)
        return false;

    char character = '\0';

    //no start quote
    if (!reader_peek(reader, &character) || character != '\"')
        return false;

    size_t string_start = reader->offset;

    reader->offset++;

    while (reader_peek(reader, &character) && character != '\"' && character != '\n' && character != '\0')
    {
        //skip next char, as it is always part of this one
        if (character == '\\')
            reader->offset++;

        reader->offset++;
    }

    //string must have an ending quote on the same line
    if (!reader_peek(reader, &character) || character != '\"')
        return false;

    //out
    if (length != NULL)
        *length = reader->offset - string_start + 1;

    //go back
    reader->offset = string_start;

    return true;
}

// Parse next double-quoted json-formatted string in json string.
// String must be freed once done.
// Returns true on success, and outs to string; and false on fail
static bool parse_string(struct json_reader* reader, char** string)
{
    assert(reader && string);

    //get end index of string, check whether we even have a string val
    size_t string_end = 0;

    if (!has_next_string_val(reader, &string_end))
        return false;

    //convert string value
    
    print_buffer_reset(&reader->string_buffer);
    if (!print_escaped_string_as_unescaped(&reader->string_buffer, reader_buffer_at(reader, 1), reader_buffer_at(reader, string_end - 1)))
        return false;

    char* new_string = calloc(print_buffer_length(&reader->string_buffer) + 1, sizeof(*new_string));

    //alloc fail
    if (new_string == NULL)
        return false;

    if (!print_buffer_copy_to_buffer(&reader->string_buffer, new_string, print_buffer_length(&reader->string_buffer) + 1))
    {
        free(new_string);
        return false;
    }

    //move to end of string val
    reader->offset += string_end;

    //out
    *string = new_string;

    return true;
}

// Parse next given number in the json string.
// Returns true on success, returns false on fail.
static bool parse_number(struct json_reader* reader, double* number)
{
    assert(reader && number);

    const char* buffer = reader_buffer_at(reader, 0);

    if (buffer == NULL)
        return false;

    char* endptr;
    *number = strtod(buffer, &endptr);

    //move reader to endptr (char byte after last number character)
    if (endptr != NULL)
        reader->offset += (endptr - buffer);

    return endptr != buffer && reader->offset <= reader->length;
}

// Checks whether the next characters are the given literal.
// Returns true on success, returns false on fail.
static bool has_next_literal(struct json_reader* reader, const char* literal)
{
    if (reader == NULL || literal == NULL)
        return false;

    //read characters, returning false if the characters aren't the same as in given literal until the end of the literal
    size_t i = 0;
    while (literal[i] != '\0')
    {
        if (reader_char_at(reader, i) != literal[i])
            return false; //literal not found!

        i++;
    }

    //found literal :)
    return true;
}

// Parse next bool literal in the json string.
// Returns true on success and outs boolean, returns false on fail.
static bool parse_boolean(struct json_reader* reader, bool* boolean)
{
    assert(reader && boolean);

    if (has_next_literal(reader, "true"))
    {
        *boolean = true; //out boolean
        reader->offset += 4;
        return true;
    }
    
    if (has_next_literal(reader, "false"))
    {
        *boolean = false; //out boolean
        reader->offset += 5;
        return true;
    }

    //false and true not found, not a boolean
    return false;
}

// Parses next null literal in the json string.
// Returns true if found, false if not.
static bool parse_null(struct json_reader* reader)
{
    assert(reader);

    if (has_next_literal(reader, "null"))
    {
        reader->offset += 4;
        return true;
    }
    else 
    {
        return false;
    }
}

//forward declare parsing of values

// Parses next json value in the json string.
// Val must be freed using ki_json_val_free() when done.
// Returns true on success and outs val (ki_json_val), returns false on fail.
static bool parse_value(struct json_reader* reader, struct ki_json_val** val);

// Parse next json array in the json string, using given INIT array.
// Returns true on success, returns false on fail.
static bool parse_array(struct json_reader* reader, struct ki_json_array* array)
{
    assert(reader && array);

    char character = '\0';

    //invalid json array
    if (!reader_peek(reader, &character) || character != '[')
        return false;

    //parse values

    reader->offset++; //skip first [

    reader_skip_whitespace(reader);

    while (reader_peek(reader, &character) && character != ']')
    {
        #if KI_JSON_PARSER_VERBOSE
        printf("Parsing array value index: %zu\n", array->count);
        #endif

        struct ki_json_val* val = NULL;

        //if failed to parse value, return false
        if (!parse_value(reader, &val))
            return false;

        ki_json_array_add(array, val);

        reader_skip_whitespace(reader);

        //comma separates next value
        if (!reader_peek(reader, &character) || character != ',')
            break;

        reader->offset++; //skip comma

        reader_skip_whitespace(reader);
    }

    //array never ended
    if (!reader_peek(reader, &character) || character != ']')
        return false;

    reader->offset++; //skip last ]

    return true;
}

// Parses next name of a pair in a json object.
// Name must be freed when done.
// Returns true on success and outs name, false on fail.
// Supported formats: "name" AND name
static bool parse_name(struct json_reader* reader, char** name)
{
    assert(reader && name);

    //if starts with quote, parse name just like a string instead
    if (reader_can_access(reader, 0) && reader_char_at(reader, 0) == '\"')
        return parse_string(reader, name);

    size_t name_length = 0;

    while (reader_can_access(reader, name_length) && reader_char_at(reader, name_length) != ':' && !char_is_whitespace(reader_char_at(reader, name_length)))
        name_length++;

    //name never ended
    if (!reader_can_access(reader, name_length))
        return false;

    //alloc name
    char* new_name = calloc(name_length + 1, sizeof(*new_name));

    //alloc fail
    if (new_name == NULL)
        return false;

    //read in name
    for (size_t i = 0; i < name_length; i++)
        new_name[i] = reader_char_at(reader, i);

    new_name[name_length] = '\0'; //null-terminator

    //out
    *name = new_name;

    reader->offset += name_length;

    return true;
}

// Parse next json object in the json string, using given INIT object.
// Returns true on success, returns false on fail.
static bool parse_object(struct json_reader* reader, struct ki_json_object* object)
{
    assert(reader && object);

    //invalid json object
    if (!reader_can_access(reader, 0) || reader_char_at(reader, 0) != '{')
        return false;

    //parse values
    reader->offset++; //skip first {

    reader_skip_whitespace(reader);

    while (reader_can_access(reader, 0) && reader_char_at(reader, 0) != '}')
    {
        char* name = NULL;

        if (!parse_name(reader, &name))
            return false;

        reader_skip_whitespace(reader);

        //colon separates name and value
        if (!reader_can_access(reader, 0) || reader_char_at(reader, 0) != ':')
        {
            free(name);
            return false;
        }

        #if KI_JSON_PARSER_VERBOSE
        printf("Parsing object pair: %s\n", name);
        #endif

        reader->offset++; //skip :

        reader_skip_whitespace(reader);

        //parse value

        struct ki_json_val* val = NULL;

        if (!parse_value(reader, &val))
        {
            free(name);
            return false;
        }

        ki_json_object_add(object, name, val);
        free(name); //name is copied, so we no longer need the original

        reader_skip_whitespace(reader);

        //comma separates next value
        if (!reader_can_access(reader, 0) || reader_char_at(reader, 0) != ',')
            break;

        reader->offset++; //skip comma

        reader_skip_whitespace(reader);
    }

    //object never ended
    if (!reader_can_access(reader, 0) || reader_char_at(reader, 0) != '}')
        return false;

    reader->offset++; //skip last }

    return true;
}

// Parses next json value in the json string.
// Val must be freed using ki_json_val_free() when done.
// Returns true on success and outs val (ki_json_val), returns false on fail.
static bool parse_value(struct json_reader* reader, struct ki_json_val** val)
{
    assert(reader && val);

    char character = '\0';

    if (!reader_peek(reader, &character))
        return false;

    //pick according to first character which type to try and parse, and parse it (duh)

    struct ki_json_val* new_val = calloc(1, sizeof(*new_val));

    //alloc fail
    if (new_val == NULL)
        return false;

    bool success = false;

    switch (character)
    {
        //string
        case '\"':
            new_val->type = KI_JSON_VAL_STRING;
            new_val->value.string = NULL;
            success = parse_string(reader, &new_val->value.string);
            break;
        //boolean
        case 't':
        case 'f':
            new_val->type = KI_JSON_VAL_BOOL;
            success = parse_boolean(reader, &new_val->value.boolean);
            break;   
        //json object (ki_json_object)
        case '{':
            new_val->type = KI_JSON_VAL_OBJECT;

            //alloc default capacity
            ki_json_object_init(&new_val->value.object, 5);

            success = parse_object(reader, &new_val->value.object);
            break;
        //json array (ki_json_array)
        case '[':
            new_val->type = KI_JSON_VAL_ARRAY;

            //alloc default capacity
            ki_json_array_init(&new_val->value.array, 5);

            success = parse_array(reader, &new_val->value.array);
            break;
        //null
        case 'n':
            new_val->type = KI_JSON_VAL_NULL;
            success = parse_null(reader);
            new_val->value.null = success;
            break;
        //number
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case '-':
        case 'e':
        case 'E':
        case '.':
            new_val->type = KI_JSON_VAL_NUMBER;
            success = parse_number(reader, &new_val->value.number);
            break;
        default:
            free(new_val);
            new_val = NULL;
            break;
    }

    if (success)
        *val = new_val; //out
    else if (new_val != NULL)
        ki_json_val_free(new_val);

    return success;
}

// Parse null-terminated string to a json tree.
// Val returned must be freed when done.
// Returns NULL on fail.
struct ki_json_val* ki_json_parse_string(const char* string)
{
    size_t length = strlen(string);

    struct json_reader reader = {0};
    
    if (!reader_init(&reader, string, length))
        return NULL;

    struct ki_json_val* val = NULL;
    bool success = parse_value(&reader, &val);
    
    reader_fini(&reader);

    if (!success)
    {
        if (val != NULL)
            ki_json_val_free(val);

        return NULL;
    }
    else 
    {
        return val;
    }
}

// Parse no more than n characters of string to a json tree.
// Val returned must be freed when done.
// Returns NULL on fail.
struct ki_json_val* ki_json_nparse_string(const char* string, size_t n)
{
    struct json_reader reader = {0};

    if (!reader_init(&reader, string, n))
        return NULL;

    struct ki_json_val* val = NULL;
    bool success = parse_value(&reader, &val);
    
    reader_fini(&reader);

    if (!success)
    {
        if (val != NULL)
            ki_json_val_free(val);

        return NULL;
    }
    else 
    {
        return val;
    }
}

#pragma endregion
