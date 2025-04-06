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

struct json_reader
{
    char* json_string;
    // Length of json_string (excluding null terminator)
    size_t length; 
    // Current reader index offset
    size_t offset;
};

// returns true if character is a space, horizontal tab, line feed/break or carriage return, else false.
static bool char_is_whitespace(char character)
{
    return (character == ' ') || (character == '\t') || (character == '\n') || (character == '\r');
}

#pragma region Reader

// can reader access char at index pos offsetted by the reader's offset?
static bool reader_can_access(struct json_reader* reader, size_t pos)
{
    return (reader->offset + pos < reader->length);
}

// reads char in json string at index pos offsetted by the reader's offset, returns \0 on fail
static char reader_char_at(struct json_reader* reader, size_t pos)
{
    assert(reader);

    if (!reader_can_access(reader, pos))
        return '\0';

    return reader->json_string[reader->offset + pos];
}

// returns buffer at index pos offsetted by reader's offset, NULL on fail
static char* reader_buffer_at(struct json_reader* reader, size_t pos)
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

// converts a hexidecimal digit to int, -1 if not a hexidecimal digit
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

// parse a unicode code point in a string
// only XXXX format is supported, where each X is a hex digit
// returns codepoint (unicode code point), -1 on fail
static uint32_t str_to_unicode_codepoint(const char* string)
{
    assert(string);

    uint32_t codepoint = 0;

    //read in 4 hex digits and convert them to dec
    for (int i = 0; i < 4; i++)
    {
        int digit = hex_digit_to_int(string[i]);

        //digit must be valid
        if (digit == -1)
            return -1;

        codepoint <<= 4; //shift 4 bits to left, adding 4 zero-bits at the end
        codepoint += (uint32_t)digit; //fill those zero-bits
    }

    return codepoint;
}

// convert an unicode code point to utf8 bytes
// returns number of bytes written, 0 on fail
static int unicode_codepoint_to_utf8(uint32_t codepoint, unsigned char* utf8, int buffer_size)
{
    assert(utf8);

    //oh my god text encoding really is something, isn't it
    //SEE: https://en.wikipedia.org/wiki/UTF-8#Description

    //determine amount of utf8 bytes
    
    int bytes = 0;

    if (codepoint >= 0x0000 && codepoint <= 0x007F)
        bytes = 1;
    else if (codepoint >= 0x0080 && codepoint <= 0x07FF)
        bytes = 2;
    else if (codepoint >= 0x0800 && codepoint <= 0xFFFF)
        bytes = 3;
    else if (codepoint >= 0x010000 && codepoint <= 0x10FFFF) //impossible in ki_json as only 4 hex digits are supported
        bytes = 4;

    //invalid codepoint or buffer size too small for utf8 bytes
    if (bytes == 0 || bytes > buffer_size)
        return 0;

    if (bytes == 1)
    {
        //use 7 least significant bits of codepoint
        //keep most significant bit as 0
        utf8[0] = (unsigned char)(codepoint & ((2 << 7) - 1));
        return bytes;
    }

    //set all used bytes except first byte to begin with bits 1-0
    for (int i = 1; i < bytes; i++)
        utf8[i] = 0b10 << 6;

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

// escapes given char with a backslash (n -> \n, r -> \r, ...)
// does not support \u, nor any character escape sequences that aren't used in json
// returns '\0' on fail
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

// converts escape sequence in (string) to utf8 bytes and output to (bytes) along with the length of read sequence
// supports unicode code points \uXXXX (X = hex digit), converting to utf8
// returns number of bytes written, 0 on fail
static int escape_sequence_to_utf8(char* string, unsigned char* bytes, size_t buffer_size, size_t* sequence_length)
{
    assert(string && bytes && buffer_size > 0 && sequence_length);

    //invalid escape sequence
    if (string[0] != '\\')
        return 0;

    char escape_char_type = string[1];
    if (escape_char_type == 'u') //unicode code point, convert to utf8 bytes
    {
        //parse unicode code point

        uint32_t codepoint = str_to_unicode_codepoint(string + 2);

        //invalid codepoint
        if (codepoint == -1)
            return 0;

        //convert to utf8 bytes
        
        int num_bytes = unicode_codepoint_to_utf8(codepoint, bytes, buffer_size);

        *sequence_length = 6;

        return num_bytes;
    }
    else //single char
    {
        //buffer size must be atleast 1 byte
        if (buffer_size < 1)
            return 0;

        bytes[0] = (unsigned char)char_to_single_escape_sequence_char(escape_char_type);

        *sequence_length = 2;

        return 1;
    }
}

#pragma endregion

#pragma region Parsing

// parse next quoted string in json string,
// string must be free'd once donce,
// returns true on success, and outs string and buffer size (not including null terminator); and false on fail
static bool parse_string(struct json_reader* reader, char** string, size_t* buffer_size)
{
    assert(reader && string && buffer_size);

    //not a string
    if (!reader_can_access(reader, 0) || reader_char_at(reader, 0) != '\"')
        return false;

    size_t string_start = 1;
    size_t string_end = 1;

    //get max length of string

    size_t max_string_length = 0;

    while (reader_can_access(reader, string_end) && reader_char_at(reader, string_end) != '\"')
    {
        //skip next char, as it is always part of this one
        if (reader_char_at(reader, string_end) == '\\')
            string_end++;

        max_string_length++;
        string_end++;
    }

    //string never ended
    if (!reader_can_access(reader, string_end))
        return false;

    //alloc string
    char* new_string = calloc(max_string_length + 1, sizeof(*new_string));

    if (new_string == NULL)
        return false;

    //read in string
    //parse escape sequences (will always be smaller or equal to amount of chars in original string)

    size_t reader_pos = string_start;
    size_t new_string_pos = 0;

    while (reader_pos < string_end)
    {
        size_t sequence_length = 1;

        int num_bytes = 1;
        char bytes[CHARACTER_MAX_BUFFER_SIZE]; //character bytes, max. 4 bytes (utf8)

        bytes[0] = reader_char_at(reader, reader_pos);

        //start of an escape sequence
        if (bytes[0] == '\\')
        {
            char* read_buffer = reader_buffer_at(reader, reader_pos);

            //NOTE: the cast to unsigned char* is a bit weird, but it seems to be my only choice for utf8
            num_bytes = escape_sequence_to_utf8(read_buffer, (unsigned char*)bytes, CHARACTER_MAX_BUFFER_SIZE, &sequence_length);

            //invalid escape sequence or failed to parse it
            if (num_bytes == 0)
            {
                free(new_string);
                return false;
            }
        }

        //write bytes into new string
        for (int i = 0; i < num_bytes; i++)
        {
            if (new_string_pos >= max_string_length)
            {
                free(new_string);
                return false;
            }

            new_string[new_string_pos] = bytes[i];
            new_string_pos++;
        }
        
        reader_pos += sequence_length;
    }

    //failed to read in string
    if (reader_pos < string_end)
    {
        free(new_string);
        return false;
    }

    new_string[new_string_pos] = '\0'; //null-terminator

    //move to end of string (skipping quotes)
    reader->offset += string_end + 1;

    //out
    *string = new_string;
    *buffer_size = max_string_length + 1;

    return true;
}


static bool parse_number(struct json_reader* reader, double* number)
{
    assert(reader && number);

    char* buffer = reader_buffer_at(reader, 0);

    if (buffer == NULL)
        return false;

    char* endptr;
    *number = strtod(buffer, &endptr);

    //move reader to endptr
    if (endptr != NULL)
        reader->offset += (endptr - buffer);

    return endptr != buffer;
}

// parse next given literal in the json string
// returns true on success, returns false on fail
static bool parse_literal(struct json_reader* reader, const char* literal, size_t literal_length)
{
    if (!reader_can_access(reader, literal_length - 1))
        return false;

    //read characters, returning false if the characters aren't the same as in given literal
    for (size_t i = 0; i < literal_length; i++)
    {
        if (reader_char_at(reader, i) != literal[i])
            return false; //literal not found!
    }

    //found literal :)
    reader->offset += literal_length;
    return true;
}

// parse next bool in the json string,
// returns true on success and outs boolean, returns false on fail
static bool parse_boolean(struct json_reader* reader, bool* boolean)
{
    assert(reader && boolean);

    if (!reader_can_access(reader, 0))
        return false;

    const char* check = "";
    int length = 0;
    bool new_boolean = false;
    
    //pick according to first character which string to check for
    switch (reader_char_at(reader, 0))
    {
        case 't':
            check = "true";
            length = 4;
            new_boolean = true;
            break;
        case 'f':
            check = "false";
            length = 5;
            new_boolean = false;
            break;
        default:
            return false; //invalid, not a boolean!
    }

    if (!parse_literal(reader, check, length))
        return false;

    //success! out boolean
    *boolean = new_boolean;

    return true;
}

// parses next null character sequence (as in 4 actual characters) in the json string
// returns true if found, false if not
static bool parse_null(struct json_reader* reader)
{
    assert(reader);

    return parse_literal(reader, "null", 4);
}

//forward declare parsing of values

static bool parse_value(struct json_reader* reader, struct ki_json_val** val);

// Parse next json array in the json string, using given uninit array.
// Returns true on success, returns false on fail.
static bool parse_array(struct json_reader* reader, struct ki_json_array* array)
{
    assert(reader && array);

    //invalid json array
    if (!reader_can_access(reader, 0) || reader_char_at(reader, 0) != '[')
        return false;

    //alloc default capacity
    ki_json_array_init(array, 5);

    //parse values

    reader->offset++; //skip first [

    reader_skip_whitespace(reader);

    while (reader_can_access(reader, 0) && reader_char_at(reader, 0) != ']')
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
        if (!reader_can_access(reader, 0) || reader_char_at(reader, 0) != ',')
            break;

        reader->offset++; //skip comma

        reader_skip_whitespace(reader);
    }

    //array never ended
    if (!reader_can_access(reader, 0))
        return false;

    reader->offset++; //skip last ]

    return true;
}

static bool parse_object(struct json_reader* reader, struct ki_json_object* object)
{
    assert(reader && object);

    //TODO: implement parse_object

    return false;
}

// parses next json value in the json string
// returns true on success and outs val (ki_json_val), returns false on fail
static bool parse_value(struct json_reader* reader, struct ki_json_val** val)
{
    assert(reader && val);

    if (!reader_can_access(reader, 0))
        return false;

    //pick according to first character which type to try and parse, and parse it (duh)

    struct ki_json_val* new_val = calloc(1, sizeof(*new_val));

    //alloc fail
    if (new_val == NULL)
        return false;

    bool success = false;

    switch (reader_char_at(reader, 0))
    {
        //string
        case '\"':
            new_val->type = KI_JSON_VAL_STRING;
            success = parse_string(reader, &new_val->string.bytes, &new_val->string.buffer_size);
            break;
        //boolean
        case 't':
        case 'f':
            new_val->type = KI_JSON_VAL_BOOL;
            success = parse_boolean(reader, &new_val->boolean);
            break;   
        //json object (ki_json_object)
        case '{':
            new_val->type = KI_JSON_VAL_OBJECT;
            success = parse_object(reader, &new_val->object);
            break;
        //json array (ki_json_array)
        case '[':
            new_val->type = KI_JSON_VAL_ARRAY;
            success = parse_array(reader, &new_val->array);
            break;
        //null
        case 'n':
            new_val->type = KI_JSON_VAL_NULL;
            success = parse_null(reader);
            new_val->null = success;
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
            success = parse_number(reader, &new_val->number);
            break;
        default:
            break;
    }

    if (success)
        *val = new_val; //out
    else
        free(new_val);

    return success;
}

#pragma endregion