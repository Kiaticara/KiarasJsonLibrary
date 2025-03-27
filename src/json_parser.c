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

static bool char_is_whitespace(char character)
{
    return (character == ' ') || (character == '\t') || (character == '\n');
}

static bool char_is_quote(char character)
{
    return (character == '\"') || (character == '\'');
}

//reads 1 char in json string, returns \0 if end is reached
static char reader_read_char(struct json_reader* reader)
{
    assert(reader);

    if (reader->pos == reader->length)
        return '\0';

    reader->pos++;
    return reader->json_string[reader->pos - 1];
}

//parse next quoted string in json string
//returns true on success and false if not, outs string and string length (not including null terminator)
//TODO: add escaping with backslash
static bool parse_string(struct json_reader* reader, char** string, int* length)
{
    assert(reader && string && length);
    
    int start_pos = reader->pos;
    *string = NULL;
    *length = 0;

    char quote = reader_read_char(reader);

    //invalid string, no start quote
    if (!char_is_quote(quote))
        return false;

    //get length of string
    int string_length = 0;
    char character = reader_read_char(reader);
    while (character != quote && character != '\0')
    {
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
        new_string[i] = reader_read_char(reader);

    new_string[string_length] = '\0'; //null-terminator

    //skip end quote
    reader->pos++;

    //out
    *string = new_string;
    *length = string_length;

    return true;
}

static bool parse_number(struct json_reader* reader, int* number);

static bool parse_boolean(struct json_reader* reader, bool* boolean);

static bool parse_array(struct json_reader* reader, struct ki_json_array* array);

static bool parse_object(struct json_reader* reader, struct ki_json_object* object);