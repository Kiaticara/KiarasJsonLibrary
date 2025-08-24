#include "ki_json/json_parser.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "ki_json/json.h"

// utf8 characters have 4 bytes max
#define CHARACTER_MAX_BUFFER_SIZE 4

#define IS_HIGH_SURROGATE(byte) (byte >= 0xD800 && byte <= 0xDBFF)
#define IS_LOW_SURROGATE(byte) (byte >= 0xDC00 && byte <= 0xDFFF)
#define COMBINE_SURROGATES(high, low) ((high - 0xD800) * 0x400 + (low - 0xDC00) + 0x10000);

#define CODEPOINT_REPLACEMENT_CHAR 0xFFFD

struct json_reader
{
    const char* json_string;
    // Length of json_string (excluding null terminator)
    size_t length; 
    // Current reader index offset
    size_t offset;
};

// Returns true if character is a space, horizontal tab, line feed/break or carriage return, else false.
static bool char_is_whitespace(char character)
{
    return (character == ' ') || (character == '\t') || (character == '\n') || (character == '\r');
}

/* Reader */

// Can reader access char at index pos offsetted by the reader's offset?
static bool reader_can_access(struct json_reader* reader, size_t pos)
{
    assert(reader);

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
    assert(reader && character);

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

/* Conversions */

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

// Convert an unicode code point to utf8 bytes.
// Converts invalid code points to the replacement char.
// Returns number of bytes written, 0 on fail.
static int unicode_codepoint_to_utf8(uint32_t codepoint, unsigned char* utf8, size_t buffer_size)
{
    assert(utf8);

    //oh my god text encoding really is something, isn't it
    //SEE: https://en.wikipedia.org/wiki/UTF-8#Description

    //codepoint is for null character (not supported) or outside 0x10FFFF, replace with replacement char codepoint
    if (codepoint == 0 || codepoint > 0x10FFFF)
        codepoint = CODEPOINT_REPLACEMENT_CHAR; //0xEF, 0xBF, 0xBD

    //determine amount of utf8 bytes
    
    int bytes = 0;

    if (codepoint <= 0x007F)
        bytes = 1;
    else if (codepoint >= 0x0080 && codepoint <= 0x07FF)
        bytes = 2;
    else if (codepoint >= 0x0800 && codepoint <= 0xFFFF)
        bytes = 3;
    else if (codepoint >= 0x010000 && codepoint <= 0x10FFFF)
        bytes = 4;

    //buffer size too small for utf8 bytes
    if ((size_t)bytes > buffer_size)
        return 0;

    if (bytes == 1)
    {
        //use 7 least significant bits of codepoint
        //keep most significant bit as 0
        utf8[0] = (codepoint & ((1 << 7) - 1));
        return bytes;
    }

    //set all used bytes except first byte to begin with bits 10 = 2
    for (int i = 1; i < bytes; i++)
        utf8[i] = 2 << 6;

    //least significant bits are in last byte, so start from there
    for (int i = bytes - 1; i > 0; i--)
    {
        //add 6 least significant bits of codepoint
        utf8[i] |= (codepoint & ((1 << 6) - 1));
        //shift codepoint over by read bits
        codepoint >>= 6;
    }

    //remaining bits to fill with codepoint bits
    int remaining_bits = 8 - bytes - 1;

    //first byte uses bits before first 0-bit to keep track of the amount of bytes the utf8 character uses
    //110 -> 2 bytes
    //1110 -> 3 bytes
    //11110 -> 4 bytes
    utf8[0] = ((2 << bytes) - 2) << remaining_bits;

    //fill remaining bits with remaining least significant bits of codepoint
    utf8[0] |= (codepoint & ((1 << remaining_bits) - 1));

    return bytes;
}

// Converts next utf16 literal (\uXXXX or \uXXXX\uXXXX where X is any hex digit) to codepoint, outs sequence length.
// Returns codepoint, 0 on fail.
static uint32_t utf16_literal_to_codepoint(const char* literal, const char* end, size_t* sequence_length)
{
    //surrogate pair ref: https://en.wikipedia.org/wiki/UTF-16#U+D800_to_U+DFFF_(surrogates)

    if (literal == NULL || end == NULL)
        return 0;

    if (end - literal < 6)
        return 0;

    //check for \u
    if (literal[0] != '\\' || literal[1] != 'u')
        return 0;

    uint32_t codepoint = read_hex4(literal + 2);

    //low surrogates can't be first
    if (IS_LOW_SURROGATE(codepoint))
        return 0;

    if (IS_HIGH_SURROGATE(codepoint))
    {
        if (end - literal < 12)
            return 0;

        //check for \u
        if (literal[6] != '\\' || literal[7] != 'u')
            return 0;

        uint32_t low = read_hex4(literal + 8);

        if (!IS_LOW_SURROGATE(low))
            return 0;

        codepoint = COMBINE_SURROGATES(codepoint, low);
    
        if (sequence_length != NULL)
            *sequence_length = 12;
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

    //encode codepoint as utf8 bytes into bytes buffer
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
        return utf16_literal_to_utf8(string, string_end, bytes, buffer_size, sequence_length);
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

/* Parsing */

// Checks whether reader can read in a string value.
// Returns KI_JSON_ERR_NONE if yes.
// Length includes quotation marks.
static enum ki_json_err_type has_next_string_val(struct json_reader* reader, size_t* length)
{
    if (reader == NULL)
        return KI_JSON_ERR_INTERNAL;

    char character = '\0';

    //no start quote
    if (!reader_peek(reader, &character) || character != '\"')
        return KI_JSON_ERR_UNKNOWN_TOKEN;

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
        return KI_JSON_ERR_UNTERMINATED_STRING;

    //out
    if (length != NULL)
        *length = reader->offset - string_start + 1;

    //go back
    reader->offset = string_start;

    return KI_JSON_ERR_NONE;
}

// Parse next double-quoted json-formatted string in json string.
// String must be freed once done.
static enum ki_json_err_type parse_string(struct json_reader* reader, char** string)
{
    assert(reader && string);

    //get input length and check whether we even have a string val
    size_t input_length = 0;

    enum ki_json_err_type err_type = has_next_string_val(reader, &input_length);

    if (err_type != KI_JSON_ERR_NONE)
        return err_type;

    //result will always be smaller than or the same size as the string value (excluding start + end quote) in the input json
    size_t result_max_length = input_length - 2;
    char* result = calloc(result_max_length + 1, sizeof(*result)); //include space for null-terminator
    
    if (result == NULL)
        return KI_JSON_ERR_MEMORY;
    
    const char* end = reader_buffer_at(reader, input_length - 1);
    
    reader->offset++; //skip first "
    
    const char* pos = reader_buffer_at(reader, 0);
    size_t result_index = 0;
    
    //convert escape sequences in input string
    //NOTE: only escaped utf8 characters are added in a single iteration, others are done byte per byte
    while (pos < end && result_index < result_max_length)
    {
        size_t sequence_length = 0;

        //start of an escape sequence
        if (pos[0] == '\\')
        {
            unsigned char bytes[CHARACTER_MAX_BUFFER_SIZE]; //character bytes, max. 4 bytes (utf8)
            size_t num_bytes = escape_sequence_to_utf8(pos, end, bytes, CHARACTER_MAX_BUFFER_SIZE, &sequence_length);

            //invalid escape sequence or failed to parse it
            if (num_bytes == 0)
            {
                free(result);
                return KI_JSON_ERR_INVALID_ESCAPE_SEQUENCE;
            }

            //should never occur, but in just case
            if (result_index + num_bytes >= result_max_length)
            {
                free(result);
                return KI_JSON_ERR_INTERNAL;
            }

            //add bytes into result
            for (size_t i = 0; i < num_bytes; i++)
            {
                result[result_index] = bytes[i];
                result_index++;
            }
        }
        else 
        {
            sequence_length = 1;

            result[result_index] = pos[0];
            result_index++;
        }

        pos += sequence_length;
        reader->offset += sequence_length;
    }

    result[result_index] = '\0'; //null-terminate

    reader->offset++; //skip last "

    //out
    *string = result;

    return KI_JSON_ERR_NONE;
}

// Parse next given number in the json string.
static enum ki_json_err_type parse_number(struct json_reader* reader, double* number)
{
    assert(reader && number);

    const char* buffer = reader_buffer_at(reader, 0);

    if (buffer == NULL)
        return KI_JSON_ERR_TOO_SHORT;

    char* endptr;
    *number = strtod(buffer, &endptr);

    //move reader to endptr (char byte after last number character)
    if (endptr != NULL)
        reader->offset += (endptr - buffer);

    if (endptr == buffer)
        return KI_JSON_ERR_UNKNOWN_TOKEN;
    else if (reader->offset > reader->length)
        return KI_JSON_ERR_TOO_SHORT;
    else
        return KI_JSON_ERR_NONE;
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
static enum ki_json_err_type parse_boolean(struct json_reader* reader, bool* boolean)
{
    assert(reader && boolean);

    if (has_next_literal(reader, "true"))
    {
        *boolean = true; //out boolean
        reader->offset += 4;
        return KI_JSON_ERR_NONE;
    }
    
    if (has_next_literal(reader, "false"))
    {
        *boolean = false; //out boolean
        reader->offset += 5;
        return KI_JSON_ERR_NONE;
    }

    //false and true not found, not a boolean
    return KI_JSON_ERR_UNKNOWN_TOKEN;
}

// Parses next null literal in the json string.
static enum ki_json_err_type parse_null(struct json_reader* reader)
{
    assert(reader);

    if (has_next_literal(reader, "null"))
    {
        reader->offset += 4;
        return KI_JSON_ERR_NONE;
    }
    else 
    {
        return KI_JSON_ERR_UNKNOWN_TOKEN;
    }
}

//forward declare parsing of values

// Parses next json value in the json string.
// Val must be freed using ki_json_val_free() when done.
static enum ki_json_err_type parse_value(struct json_reader* reader, struct ki_json_val** val);

// Parse next json array in the json string, using given INIT array.
static enum ki_json_err_type parse_array(struct json_reader* reader, struct ki_json_array* array)
{
    assert(reader && array);

    char character = '\0';

    //invalid json array
    if (!reader_peek(reader, &character) || character != '[')
        return KI_JSON_ERR_UNKNOWN_TOKEN;

    //parse values

    reader->offset++; //skip first [

    reader_skip_whitespace(reader);

    bool value_expected = false;
    size_t pos_comma = 0;

    while (reader_peek(reader, &character) && character != ']')
    {
        #if KI_JSON_PARSER_VERBOSE
        printf("Parsing array value index: %zu\n", array->count);
        #endif

        struct ki_json_val* val = NULL;
        enum ki_json_err_type err_type = parse_value(reader, &val);

        if (err_type != KI_JSON_ERR_NONE)
            return err_type;

        err_type = ki_json_array_add(array, val);

        if (err_type != KI_JSON_ERR_NONE)
            return err_type;

        reader_skip_whitespace(reader);

        //comma separates next value
        if (reader_char_at(reader, 0) == ',')
        {
            pos_comma = reader->offset;
            reader->offset++; //skip comma
            reader_skip_whitespace(reader);
            value_expected = true;
        }
        else
        {
            value_expected = false;
        }
    }

    if (value_expected)
    {
        reader->offset = pos_comma; //go back to comma
        return KI_JSON_ERR_TRAILING_COMMA;
    }

    //array never ended
    if (!reader_peek(reader, &character) || character != ']')
        return KI_JSON_ERR_UNTERMINATED_ARRAY;

    reader->offset++; //skip last ]

    return KI_JSON_ERR_NONE;
}

// Parse next json object in the json string, using given INIT object.
static enum ki_json_err_type parse_object(struct json_reader* reader, struct ki_json_object* object)
{
    assert(reader && object);

    //invalid json object
    if (!reader_can_access(reader, 0) || reader_char_at(reader, 0) != '{')
        return KI_JSON_ERR_UNKNOWN_TOKEN;

    //parse values
    reader->offset++; //skip first {

    reader_skip_whitespace(reader);

    bool pair_expected = false;
    size_t pos_comma = 0;

    while (reader_can_access(reader, 0) && reader_char_at(reader, 0) != '}')
    {
        enum ki_json_err_type err_type = KI_JSON_ERR_NONE;

        char* name = NULL;

        err_type = parse_string(reader, &name);

        if (err_type == KI_JSON_ERR_UNKNOWN_TOKEN)
            return KI_JSON_ERR_EXPECTED_NAME;
        else if (err_type != KI_JSON_ERR_NONE)
            return err_type;

        reader_skip_whitespace(reader);

        //colon separates name and value
        if (!reader_can_access(reader, 0) || reader_char_at(reader, 0) != ':')
        {
            free(name);
            return KI_JSON_ERR_EXPECTED_NAME_VALUE_SEPARATOR;
        }

        #if KI_JSON_PARSER_VERBOSE
        printf("Parsing object pair: %s\n", name);
        #endif

        reader->offset++; //skip :

        reader_skip_whitespace(reader);

        //parse value

        struct ki_json_val* val = NULL;

        err_type = parse_value(reader, &val);

        if (err_type != KI_JSON_ERR_NONE)
        {
            free(name);
            return err_type;
        }

        err_type = ki_json_object_add(object, name, val);
        free(name); //name is copied, so we no longer need the original

        if (err_type != KI_JSON_ERR_NONE)
        {
            ki_json_val_free(val);
            return err_type;
        }

        reader_skip_whitespace(reader);

        //comma separates next pair
        if (reader_char_at(reader, 0) == ',')
        {
            pos_comma = reader->offset;
            reader->offset++; //skip comma
            reader_skip_whitespace(reader);
            pair_expected = true;
        }
        else
        {
            pair_expected = false;
        }
    }

    if (pair_expected)
    {
        reader->offset = pos_comma; //go back to comma
        return KI_JSON_ERR_TRAILING_COMMA;
    }

    //object never ended
    if (!reader_can_access(reader, 0) || reader_char_at(reader, 0) != '}')
        return KI_JSON_ERR_UNTERMINATED_OBJECT;

    reader->offset++; //skip last }

    return KI_JSON_ERR_NONE;
}

// Parses next json value in the json string.
// Val must be freed using ki_json_val_free() when done.
static enum ki_json_err_type parse_value(struct json_reader* reader, struct ki_json_val** val)
{
    assert(reader && val);

    char character = '\0';

    if (!reader_peek(reader, &character))
        return KI_JSON_ERR_TOO_SHORT;

    //pick according to first character which type to try and parse, and parse it (duh)

    struct ki_json_val* new_val = calloc(1, sizeof(*new_val));

    //alloc fail
    if (new_val == NULL)
        return KI_JSON_ERR_MEMORY;

    enum ki_json_err_type err_type = KI_JSON_ERR_NONE;

    switch (character)
    {
        //string
        case '\"':
            new_val->type = KI_JSON_VAL_STRING;
            new_val->value.string = NULL;
            err_type = parse_string(reader, &new_val->value.string);
            break;
        //boolean
        case 't':
        case 'f':
            new_val->type = KI_JSON_VAL_BOOL;
            err_type = parse_boolean(reader, &new_val->value.boolean);
            break;   
        //json object (ki_json_object)
        case '{':
            new_val->type = KI_JSON_VAL_OBJECT;

            //alloc default capacity
            ki_json_object_init(&new_val->value.object, 5);

            err_type = parse_object(reader, &new_val->value.object);
            break;
        //json array (ki_json_array)
        case '[':
            new_val->type = KI_JSON_VAL_ARRAY;

            //alloc default capacity
            ki_json_array_init(&new_val->value.array, 5);

            err_type = parse_array(reader, &new_val->value.array);
            break;
        //null
        case 'n':
            new_val->type = KI_JSON_VAL_NULL;
            err_type = parse_null(reader);
            new_val->value.null = true;
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
        case '.':
            new_val->type = KI_JSON_VAL_NUMBER;
            err_type = parse_number(reader, &new_val->value.number);
            break;
        default:
            free(new_val);
            new_val = NULL;
            err_type = KI_JSON_ERR_UNKNOWN_TOKEN;
            break;
    }

    if (err_type == KI_JSON_ERR_NONE)
        *val = new_val; //out
    else if (new_val != NULL)
        ki_json_val_free(new_val);

    return err_type;
}

// Parse null-terminated string to a json tree.
// Val returned must be freed using ki_json_val_free() when done.
// Returns NULL on fail and outs error to err.
struct ki_json_val* ki_json_parse_string(const char* string, struct ki_json_parser_err* err)
{
    return ki_json_nparse_string(string, strlen(string), err);
}

// Parse no more than n characters of string to a json tree.
// Val returned must be freed using ki_json_val_free() when done.
// Returns NULL on fail and outs error to err.
struct ki_json_val* ki_json_nparse_string(const char* string, size_t n, struct ki_json_parser_err* err)
{
    if (err != NULL)
    {
        err->json = string;
        err->pos = 0;
        err->type = KI_JSON_ERR_INTERNAL;
    }

    if (string == NULL)
    {
        if (err != NULL)
            err->type = KI_JSON_ERR_INVALID_ARGS;
        
        return NULL;
    }

    struct json_reader reader = {
        .json_string = string,
        .length = n,
        .offset = 0
    };

    //skip byte order mark if necessary
    if (has_next_literal(&reader, "\uFEFF"))
        reader.offset += 3;

    struct ki_json_val* val = NULL;
    enum ki_json_err_type err_type = parse_value(&reader, &val);

    if (err != NULL)
    {
        err->pos = reader.offset;
        err->type = err_type;
    }

    if (err_type != KI_JSON_ERR_NONE)
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

