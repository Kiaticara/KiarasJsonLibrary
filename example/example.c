#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include "ki_json/json_node.h"
#include "ki_json/json_object.h"

struct json_reader
{
    char* json_string;
    int length;

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

    struct json_reader test_reader = {"\"test lol lol\"\"can't see me yet!\"\"tab\\tta\\nb\" 'crash jk'truefalse tRuEFALSEtrue", 79, 0};

    char* string = NULL;
    int length = 0;

    //read 5 strings, third string will be fail as it has no start quote (just a space)
    for (int i = 0; i < 5; i++)
    {
        if (parse_string(&test_reader, &string, &length))
            printf("read string %i: %s (length = %i) (pos = %i)\n", i + 1, string, length, test_reader.pos);
        else
            printf("read string %i failed (length = %i) (pos = %i)\n", i + 1, length, test_reader.pos);
    }

    //read 6 bools, only first 2 should be successful
    bool boolean = false;
    for (int i = 0; i < 6; i++)
    {
        if (parse_boolean(&test_reader, &boolean))
            printf("read bool %i: %i\n", i + 1, boolean);
        else
            printf("read bool %i failed\n", i + 1);
    }

    return 0;
}