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

    struct json_reader test_reader = {"\"test lol lol\"\"can't see me yet!\" 'crash jk'", 44, 0};

    char* string = NULL;
    int length = 0;

    //read four strings, third string will be fail as it has no start quote (just a space)
    for (int i = 0; i < 4; i++)
    {
        if (parse_string(&test_reader, &string, &length))
            printf("read string %i: %s (length = %i) (pos = %i)\n", i + 1, string, length, test_reader.pos);
        else
            printf("read string %i is null (length = %i) (pos = %i)\n", i + 1, length, test_reader.pos);
    }

    return 0;
}