#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

#include "ki_json/json.h"
#include "ki_json/json_parser.h"

//TODO: remove this when finished
//Just for testing static parser functions
#include "../src/json_parser.c"

int main(void)
{
    //TODO: example should do null checks for json objects & nodes
    
    printf("creating json object\n");

    struct ki_json_val* val_object = ki_json_val_create_object(1);

    printf("creating null json val\n");

    struct ki_json_val* val_null = ki_json_val_create_null();
    
    printf("creating 2 number json values...\n");

    struct ki_json_val* val_num1 = ki_json_val_create_from_number(5.2);
    struct ki_json_val* val_num2 = ki_json_val_create_from_number(-202.2);
    
    printf("number node 1 val: %f\n", val_num1->value.number);
    printf("number node 2 val: %f\n", val_num2->value.number);

    printf("adding existing values to json object\n");

    ki_json_object_add(&val_object->value.object, "test1", val_null);
    ki_json_object_add(&val_object->value.object, "test2", val_num1);
    ki_json_object_add(&val_object->value.object, "test3", val_num2);

    printf("adding new nodes to json object\n");

    ki_json_object_add_new_bool(&val_object->value.object, "test4", true);
    ki_json_object_add_new_string(&val_object->value.object, "test5", "abc");
    ki_json_object_add_new_number(&val_object->value.object, "test6", 3.3);
    
    printf("destroying json object val\n");
    ki_json_val_free(val_object);

    struct json_reader test_reader = {"\"test lol lol\"\"can't see me yet!\"\"tab\\tta\\nb\\t\"\"aa\\u00AEabc\"truefalse true2.234-99.92.2", 87, 0};

    char* string = NULL;
    size_t buffer_size = 0;

    //read 5 strings, 5th string will be fail as it has no start quote
    for (int i = 0; i < 5; i++)
    {
        if (parse_string(&test_reader, &string, &buffer_size))
        {
            printf("read string %i: %s (buffer_size = %zu) (length = %zu) (offset = %zu)\n", i + 1, string, buffer_size, strlen(string), test_reader.offset);
            free(string);
        }
        else
        {
            printf("read string %i failed (offset = %zu)\n", i + 1, test_reader.offset);
        }
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

    //unicode code point testing
    //first: 2 byte trademark character & 2 byte yen sign
    //second: 2 byte Latin Capital Letter Esh and 3 byte Latin Small Letter Y with loop and another 2 byte Latin Capital Letter Esh

    struct json_reader reader2 = {"\"aa\\u00AEabc\\u00A5\"\"\\u01A9\\u1EFF\\u01A9\"", 39, 0};
    
    string = NULL;
    buffer_size = 0;

    //read 3 strings, after 2nd will fail
    for (int i = 0; i < 5; i++)
    {
        if (parse_string(&reader2, &string, &buffer_size))
        {
            printf("read string %i: %s (buffer_size = %zu) (length = %zu) (offset = %zu)\n", i + 1, string, buffer_size, strlen(string), reader2.offset);
            free(string);
        }
        else
        {
            printf("read string %i failed (offset = %zu)\n", i + 1, reader2.offset);
        }
    }

    return 0;
}
