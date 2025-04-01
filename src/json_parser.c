#include "ki_json/json_parser.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

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
static bool reader_can_read(struct json_reader* reader, size_t num)
{
    return (reader->offset + num <= reader->length);
}

// can reader access char at index pos offsetted by the reader's offset?
static bool reader_can_access_char(struct json_reader* reader, size_t pos)
{
    return (reader->offset + pos < reader->length);
}

// reads char in json string at index pos offsetted by the reader's offset, returns \0 on fail
static char reader_access_char(struct json_reader* reader, size_t pos)
{
    assert(reader);

    if (!reader_can_access_char(reader, pos))
        return '\0';

    return reader->json_string[reader->offset + pos];
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

// parses next null character sequence (as in 4 actual characters) in the json string
// returns true if found, false if not
static bool parse_null(struct json_reader* reader)
{
    assert(reader);

    return parse_literal(reader, "null", 4);
}
