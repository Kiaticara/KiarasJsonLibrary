#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

#include "ki_json/json_node.h"
#include "ki_json/json_object.h"

#define CHARACTER_MAX_BUFFER_SIZE 5

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

// can reader read (num) amount of chars?
static bool reader_can_read(struct json_reader* reader, int num)
{
    return (reader->offset + num <= reader->length);
}

// can reader access char at index pos offsetted by the reader's offset?
static bool reader_can_access_char(struct json_reader* reader, int pos)
{
    return (reader->offset + pos < reader->length);
}

// reads char in json string at index pos offsetted by the reader's offset, returns \0 on fail
static char reader_access_char(struct json_reader* reader, int pos)
{
    assert(reader);

    if (!reader_can_access_char(reader, pos))
        return '\0';

    return reader->json_string[reader->offset + pos];
}

static char* reader_buffer_at(struct json_reader* reader, int pos)
{
    assert(reader);

    if (!reader_can_access_char(reader, pos))
        return NULL;

    return reader->json_string + reader->offset + pos;
}

// reads 1 char in json string, returns \0 if end is reached
// DEPRECATED DON'T USE
static char reader_read_char(struct json_reader* reader)
{
    assert(reader);

    if (reader->offset == reader->length)
        return '\0';

    reader->offset++;
    return reader->json_string[reader->offset - 1];
}

static void reader_skip_whitespace(struct json_reader* reader)
{
    assert(reader);

    while (reader_can_read(reader, 1) && char_is_whitespace(reader_access_char(reader, 0)))
        reader->offset++;
}

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

// parse a unicode code point in json string
// only XXXX format is supported, where each X is a hex digit
// returns true on success, and outs codepoint (unicode code point), and false on fail
static bool parse_unicode_codepoint(struct json_reader* reader, uint32_t* codepoint)
{
    assert(reader && codepoint);

    if (!reader_can_read(reader, 4))
        return false;

    uint32_t new_codepoint = 0;

    //read in 4 hex digits and convert them to dec
    for (int i = 0; i < 4; i++)
    {
        int digit = hex_digit_to_int(reader_access_char(reader, i));

        //digit must be valid
        if (digit == -1)
            return false;

        new_codepoint <<= 4; //shift 4 bits to left, adding 4 zero-bits at the end
        new_codepoint += (uint32_t)digit; //fill those zero-bits
    }

    //out
    *codepoint = new_codepoint;

    return true;
}

// convert an unicode code point to utf8 bytes, with a null-terminator
// returns true on success, and outs utf8 bytes, and false on fail
static bool unicode_codepoint_to_utf8(uint32_t codepoint, char* utf8, int buffer_size)
{
    assert(utf8);

    // determine amount of utf8 bytes
    // SEE: https://en.wikipedia.org/wiki/UTF-8#Description

    int bytes = -1;

    if (codepoint >= 0x0000 && codepoint <= 0x007F)
        bytes = 1;
    else if (codepoint >= 0x0080 && codepoint <= 0x07FF)
        bytes = 2;
    else if (codepoint >= 0x0800 && codepoint <= 0xFFFF)
        bytes = 3;
    else if (codepoint >= 0x010000 && codepoint <= 0x10FFFF) //impossible in ki_json as only 4 hex digits are supported
        bytes = 4;

    //invalid codepoint or buffer size too small for utf8 char + null-terminator
    if (bytes == -1 || bytes + 1 < buffer_size)
        return false;

    //TODO: encode to utf8 bytes
    
    return false;
}

// escapes given char with a backslash (n -> \n, r -> \r, ...)
// does not support \u, nor any single character escape sequences that aren't used in json
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

// outputs character bytes of escape sequence to (bytes) with null-terminator and length of read sequence, with buffer size
// supports unicode code points \uXXXX (X = hex digit), converting to utf8
// returns true on success, false on fail.
static bool parse_escape_sequence(struct json_reader* reader, char* bytes, int buffer_size, int* sequence_length)
{
    assert(reader && bytes && buffer_size > 0 && sequence_length);

    //reader must be able to read atleast 2 chars
    if (!reader_can_read(reader, 2))
        return false;

    //invalid escape sequence
    if (reader_access_char(reader, 0) != '\\')
        return false;

    char escape_char_type = reader_access_char(reader, 1);
    if (escape_char_type == 'u') //unicode code point, convert to utf8 bytes
    {
        //parse unicode code point

        uint32_t codepoint = -1;

        reader->offset += 2;
        bool codepoint_success = parse_unicode_codepoint(reader, &codepoint);
        reader->offset -= 2; //backtrack

        if (!codepoint_success)
            return false;

        //convert to utf8 bytes
        
        if (!unicode_codepoint_to_utf8(codepoint, bytes, buffer_size))
            return false;

        *sequence_length = 6;
    }
    else //single char
    {
        //buffer size must be atleast 2 bytes
        if (buffer_size < 2)
            return false;

        bytes[0] = char_to_single_escape_sequence_char(escape_char_type);
        bytes[1] = '\0'; //null-terminator

        *sequence_length = 2;
    }

    return true;
}

// parse next quoted string in json string,
// string must be free'd,
// returns true on success, and outs string and string length (not including null terminator); and false on fail
static bool parse_string(struct json_reader* reader, char** string, int* length)
{
    assert(reader && string && length);

    //not a string
    if (!reader_can_access_char(reader, 0) || reader_access_char(reader, 0) != '\"')
        return false;

    int string_start = 1;
    int string_end = 1;

    //get max length of string
    int string_length = 0;

    while (reader_can_access_char(reader, string_end) && reader_access_char(reader, string_end) != '\"')
    {
        //skip next char, as it is always part of this one
        if (reader_access_char(reader, string_end) == '\\')
            string_end++;

        string_length++;
        string_end++;
    }

    //string never ended
    if (!reader_can_access_char(reader, string_end))
        return false;

    //alloc string
    char* new_string = calloc(string_length + 1, sizeof(char));

    if (new_string == NULL)
        return false;

    //read in string
    //parse escape sequences (will always be smaller or equal to amount of chars in original string)

    int reader_pos = string_start;
    int new_string_pos = 0;

    while (reader_pos < string_end && new_string_pos < string_length)
    {
        int read_sequence_length = 1;
        char bytes[CHARACTER_MAX_BUFFER_SIZE]; //character bytes, max. 4 bytes (utf8) + 1 for null-terminator
        bytes[0] = reader_access_char(reader, reader_pos);
        bytes[1] = '\0';

        //start of an escape sequence
        if (bytes[0] == '\\')
        {
            reader->offset += reader_pos;
            
            //invalid escape sequence or failed to parse it
            if (!parse_escape_sequence(reader, bytes, CHARACTER_MAX_BUFFER_SIZE, &read_sequence_length))
            {
                free(new_string);
                return false;
            }

            reader->offset -= reader_pos; //backtrack
        }

        //last byte is always a null-terminator, or completely empty if the null-terminator appears earlier
        //this ensures that strlen will never go over 4
        //we do this every iteration just in case somewhere we slip up and edit the last byte
        //and i'm very scared of slipping into the volcano known as memory corruption
        bytes[4] = '\0';

        int length = (int)strlen(bytes);
        for (int i = 0; i < length; i++)
        {
            if (new_string_pos >= string_length)
            {
                free(new_string);
                return false;
            }

            new_string[new_string_pos] = bytes[i];
            new_string_pos++;
        }
        
        reader_pos += read_sequence_length;
    }

    //failed to read in string
    if ((new_string_pos >= string_length && reader_pos < string_end) || reader_pos > string_end)
    {
        free(new_string);
        return false;
    }

    //move to end of string (skipping quotes)
    reader->offset += string_end + 1;

    //out
    *string = new_string;
    *length = string_length;

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
static bool parse_literal(struct json_reader* reader, const char* literal, int literal_length)
{
    if (!reader_can_read(reader, literal_length))
        return false;

    //read characters, returning false if the characters aren't the same as in given literal
    for (int i = 0; i < literal_length; i++)
    {
        if (reader_access_char(reader, i) != literal[i])
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

    if (!reader_can_read(reader, 1))
        return false;

    const char* check = "";
    int length = 0;
    bool new_boolean = false;
    
    //pick according to first character which string to check for
    switch (reader_access_char(reader, 0))
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

int main()
{
    //TODO: example should do null checks for json objects & nodes
    
    printf("creating json object\n");

    struct ki_json_object* object = ki_json_object_create(1);

    printf("creating null node\n");

    struct ki_json_node* node_null = ki_json_node_create_null();
    
    printf("creating 2 number node...\n");

    struct ki_json_node* node_num1 = ki_json_node_create_from_number(5.2);
    struct ki_json_node* node_num2 = ki_json_node_create_from_number(-202.2);
    
    printf("number node 1 val: %f\n", (float)ki_json_node_get_number(node_num1));

    printf("number node 2 val: %f\n", (float)ki_json_node_get_number(node_num2));

    printf("adding existing nodes to json object\n");

    ki_json_object_add_node(object, "test1", node_null);
    ki_json_object_add_node(object, "test2", node_num1);
    ki_json_object_add_node(object, "test3", node_num2);

    printf("adding new nodes to json object\n");

    ki_json_object_add_bool(object, "test4", true);
    ki_json_object_add_string(object, "test5", "abc");
    ki_json_object_add_number(object, "test6", 3.3);
    
    printf("destroying json object\n");
    ki_json_object_free(object);

    struct json_reader test_reader = {"\"test lol lol\"\"can't see me yet!\"\"tab\\tta\\nb\\t\"truefalse true2.234-99.92.2", 74, 0};

    char* string = NULL;
    int length = 0;

    //read 5 strings, 4th and 5th string will be fail as they have no start quote
    for (int i = 0; i < 5; i++)
    {
        if (parse_string(&test_reader, &string, &length))
            printf("read string %i: %s (length = %i) (offset = %zu)\n", i + 1, string, length, test_reader.offset);
        else
            printf("read string %i failed (length = %i) (offset = %zu)\n", i + 1, length, test_reader.offset);
    }

    //read 3 bools, only first 2 should be successful
    bool boolean = false;
    for (int i = 0; i < 3; i++)
    {
        if (parse_boolean(&test_reader, &boolean))
            printf("read bool %i: %i\n", i + 1, boolean);
        else
            printf("read bool %i failed\n", i + 1);
    }

    printf("skipping whitespace...\n");

    reader_skip_whitespace(&test_reader);

    if (parse_boolean(&test_reader, &boolean))
        printf("read bool %i: %i\n", 4, boolean);
    else
        printf("read bool %i failed\n", 4);

    double number = 0.0;
    for (int i = 0; i < 3; i++)
    {
        if (parse_number(&test_reader, &number))
            printf("read number %i: %f\n", i + 1, number);
        else
            printf("read number %i failed\n", i + 1);
    }

    return 0;
}