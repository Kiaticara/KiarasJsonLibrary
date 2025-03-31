#include "ki_json/json_parser.h"

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

struct json_reader
{
    char* json_string;
    // Length of json_string (excluding null terminator)
    int length; 
    // Current reader index position
    int pos;
};

// returns true if character is a space, horizontal tab, line feed/break or carriage return, else false.
static bool char_is_whitespace(char character)
{
    return (character == ' ') || (character == '\t') || (character == '\n') || (character == '\r');
}

static bool char_is_quote(char character)
{
    return (character == '\"') || (character == '\'');
}

// reads 1 char in json string, returns \0 if end is reached
static char reader_read_char(struct json_reader* reader)
{
    assert(reader);

    if (reader->pos == reader->length)
        return '\0';

    reader->pos++;
    return reader->json_string[reader->pos - 1];
}

// reads an escaped sequence in json string, returns \0 if end is reached or on fail.
// TODO: implement \u: unicode code point support (4 hex digits only)
static char parse_escape_sequence(struct json_reader* reader)
{
    assert(reader);

    //escaped chars start with backward slashes
    if (reader_read_char(reader) != '\\')
        return '\0';

    switch(reader_read_char(reader)) //escape char type
    {
        case '\'': //single quotation mark
            return '\"';
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
        case 'u': //unicode code point, TODO: +4 hex digits
            return '\0';
        default:
            return '\0';
    }
}

// parse next quoted string in json string,
// string must be free'd,
// returns true on success, and outs string and string length (not including null terminator); and false on fail
static bool parse_string(struct json_reader* reader, char** string, int* length)
{
    assert(reader && string && length);
    
    int start_pos = reader->pos;

    char quote = reader_read_char(reader);

    //invalid string, no start quote or end of json string reached
    if (!char_is_quote(quote))
        return false;

    //get length of string
    int string_length = 0;
    char character = reader_read_char(reader);
    while (character != quote && character != '\0')
    {
        //start of an escaped sequence
        if (character == '\\')
        {
            reader->pos--;
            character = parse_escape_sequence(reader);

            //invalid escape sequence or end of json string reached
            if (character == '\0')
                return false;
        }

        string_length++;
        character = reader_read_char(reader);
    }

    //invalid string, must have an end quote that is the same as the start quote
    if (character != quote)
        return false;

    //alloc string
    char* new_string = calloc(string_length + 1, sizeof(char));

    if (new_string == NULL)
        return false;

    //read in string
    reader->pos = start_pos + 1;
    for (int i = 0; i < string_length; i++)
    {
        character = reader_read_char(reader);

        //start of an escape sequence
        if (character == '\\')
        {
            reader->pos--;
            character = parse_escape_sequence(reader);
        }

        new_string[i] = character;
    }

    new_string[string_length] = '\0'; //null-terminator

    //skip end quote
    reader->pos++;

    //out
    *string = new_string;
    *length = string_length;

    return true;
}

static bool parse_number(struct json_reader* reader, int* number);

// parse next bool in the json string,
// returns true on success and outs boolean, returns false on fail
static bool parse_boolean(struct json_reader* reader, bool* boolean)
{
    assert(reader && boolean);

    bool new_boolean = false; 
    const char* check = ""; //string to check for
    int length = 0; //length of string to check for

    //pick according to first character which string to check for
    switch (reader_read_char(reader))
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

    //check for the rest of the characters
    for (int i = 1; i < length; i++)
    {
        if (reader_read_char(reader) != check[i])
            return false; //invalid, not a boolean!
    }

    //success! out boolean
    *boolean = new_boolean;

    return true;
}

static bool parse_array(struct json_reader* reader, struct ki_json_array* array);

static bool parse_object(struct json_reader* reader, struct ki_json_object* object);