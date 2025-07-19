#include "ki_json/json_generator.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "print_buffer.h"
#include "ki_json/json.h"

struct json_generator
{
    struct print_buffer buffer;
    int depth;
};

#pragma region Printing

// Prints json-formatted character into print buffer, through an escape sequence.
// Returns true on success, and false on fail.
static bool print_escape_sequence(struct print_buffer* buffer, unsigned char character)
{
    if (buffer == NULL)
        return false;

    switch(character)
    {
        case '\"': //double quotation marks
            return print_buffer_append_string(buffer, "\\\"");
        case '\\': //reverse solidus
            return print_buffer_append_string(buffer, "\\\\");
        case '\b': //backspace
            return print_buffer_append_string(buffer, "\\b");
        case '\f': //form feed
            return print_buffer_append_string(buffer, "\\f");
        case '\n': //line feed, line break
            return print_buffer_append_string(buffer, "\\n");
        case '\r': //carriage return
            return print_buffer_append_string(buffer, "\\r");
        case '\t': //horizontal tab
            return print_buffer_append_string(buffer, "\\t");
        default:
        {
            char escaped[7]; // \uXXXX\0
            snprintf(escaped, sizeof(escaped), "\\u%04.4X", character);
            return print_buffer_append_string(buffer, escaped);
        }
    }
}

// Prints json-formatted string into print buffer.
// Returns true on success, and false on fail.
static bool print_string(struct print_buffer* buffer, const char* string)
{
    if (buffer == NULL || string == NULL)
        return false;

    //start double quote
    if (!print_buffer_append_char(buffer, '\"'))
        return false;

    size_t pos = 0;
    
    while (string[pos] != '\0')
    {
        if ((string[pos] >= 0x0000 && string[pos] <= 0x001F) || string[pos] == '\"' || string[pos] == '\\')
        {
            if (!print_escape_sequence(buffer, (unsigned char)string[pos]))
                return false;
        }
        else if (!print_buffer_append_char(buffer, string[pos]))
        {
            return false;
        }

        pos++;
    }

    //end double quote
    if (!print_buffer_append_char(buffer, '\"'))
        return false;

    return true;
}

// Prints number into print buffer.
// Returns true on success, and false on fail.
static bool print_number(struct print_buffer* buffer, double number)
{
    if (buffer == NULL)
        return false;

    const size_t NUM_MAX_LENGTH = 128;
    
    char number_string[NUM_MAX_LENGTH];
    size_t length = snprintf(number_string, NUM_MAX_LENGTH - 1, "%g", number);

    if (length == 0 || length >= NUM_MAX_LENGTH)
        return false;

    return print_buffer_append_string(buffer, number_string);
}

// Prints boolean into print buffer.
// Returns true on success, and false on fail.
static bool print_boolean(struct print_buffer* buffer, bool boolean)
{
    if (buffer == NULL)
        return false;

    return print_buffer_append_string(buffer, boolean ? "true" : "false");
}

// Prints null (n-u-l-l, not the NULL character) into print buffer.
// Returns true on success, and false on fail.
static bool print_null(struct print_buffer* buffer)
{
    if (buffer == NULL)
        return false;

    return print_buffer_append_string(buffer, "null");
}

// forward declare printing of values

// Prints json value into generator's print buffer.
// Returns true on success, and false on fail.
static bool print_value(struct json_generator* generator, struct ki_json_val* val);

static bool print_depth(struct print_buffer* buffer, int depth)
{
    if (buffer == NULL)
        return false;

    for (int i = 0; i < depth; i++)
    {
        if (!print_buffer_append_char(buffer, '\t'))
            return false;
    }
    
    return true;
}

// Prints json array into generator's print buffer.
// Returns true on success, and false on fail.
static bool print_array(struct json_generator* generator, struct ki_json_array* array)
{
    if (generator == NULL || array == NULL)
        return false;

    if (!print_buffer_append_string(&generator->buffer, "[\n"))
        return false;

    generator->depth++;

    for (size_t i = 0; i < array->count; i++)
    {
        if (!print_depth(&generator->buffer, generator->depth))
            return false;

        if (!print_value(generator, array->values[i]))
            return false; 

        if (i != array->count - 1 && !print_buffer_append_char(&generator->buffer, ','))
            return false;
        
        if (!print_buffer_append_char(&generator->buffer, '\n'))
            return false;
    }

    generator->depth--;
    
    if (!print_depth(&generator->buffer, generator->depth))
        return false;

    if (!print_buffer_append_char(&generator->buffer, ']'))
        return false;

    return true;
}

// TODO: should probably find a way to not have this mess of if-statements
// Prints json object into generator's print buffer.
// Returns true on success, and false on fail.
static bool print_object(struct json_generator* generator, struct ki_json_object* object)
{
    if (generator == NULL || object == NULL)
        return false;

    if (!print_buffer_append_string(&generator->buffer, "{\n"))
        return false;

    generator->depth++;

    for (size_t i = 0; i < object->count; i++)
    {
        if (!print_depth(&generator->buffer, generator->depth))
            return false;
        
        if (!print_string(&generator->buffer, object->names[i]))
            return false;

        if (!print_buffer_append_string(&generator->buffer, ": "))
            return false;
        
        if (!print_value(generator, object->values[i]))
            return false; 

        if (i != object->count - 1 && !print_buffer_append_char(&generator->buffer, ','))
            return false;
        
        if (!print_buffer_append_char(&generator->buffer, '\n'))
            return false;
    }

    generator->depth--;

    if (!print_depth(&generator->buffer, generator->depth))
        return false;

    if (!print_buffer_append_char(&generator->buffer, '}'))
        return false;

    return true;
}

// Prints json value into generator's print buffer.
// Returns true on success, and false on fail.
static bool print_value(struct json_generator* generator, struct ki_json_val* val)
{
    if (generator == NULL || val == NULL)
        return false;

    switch(val->type)
    {
        case KI_JSON_VAL_STRING:
            return print_string(&generator->buffer, val->value.string);
        case KI_JSON_VAL_NUMBER:
            return print_number(&generator->buffer, val->value.number);
        case KI_JSON_VAL_BOOL:
            return print_boolean(&generator->buffer, val->value.boolean);
        case KI_JSON_VAL_NULL:
            return print_null(&generator->buffer);
        case KI_JSON_VAL_ARRAY:
            return print_array(generator, &val->value.array);
        case KI_JSON_VAL_OBJECT:
            return print_object(generator, &val->value.object);
        default:
            return false;
    }
}

// Generate string from json val.
// Returned string must be freed once done.
// Returns NULL on fail.
char* ki_json_gen_string(struct ki_json_val* val)
{
    if (val == NULL)
        return NULL;

    struct json_generator generator = {0};

    if (!print_buffer_init(&generator.buffer, 256))
        return NULL;

    generator.depth = 0;

    if (!print_value(&generator, val))
        return NULL;

    char* string = calloc(print_buffer_length(&generator.buffer) + 1, sizeof(*string));

    if (string == NULL)
    {
        print_buffer_fini(&generator.buffer);
        return NULL;
    }

    if (!print_buffer_copy_to_buffer(&generator.buffer, string, print_buffer_length(&generator.buffer) + 1))
    {
        print_buffer_fini(&generator.buffer);
        free(string);
        return NULL;
    }

    print_buffer_fini(&generator.buffer);

    return string;
}
