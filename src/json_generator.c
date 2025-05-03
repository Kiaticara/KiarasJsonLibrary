#include "ki_json/json_generator.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "print_buffer.h"
#include "ki_json/json.h"

struct json_generator
{
    struct print_buffer buffer;
    int depth;
};

#pragma region utf-8

// Returns the amount of bytes a utf8 character will have from the starting byte.
// Returns 0 if invalid, or -1 if a byte is given that is not the starting byte.
static int utf8_amount_of_bytes(unsigned char byte)
{
    //leading 1-bits before first 0-bit determines amount of bytes
    //0... (no 1-bits) -> 1 byte (ASCII char)
    //10 (1 1-bit) -> not first byte
    //110... (2 1-bits) -> 2 bytes
    //1110... (3 1-bits) -> 3 bytes
    //11110 (4 1-bits) -> 4 bytes

    if (byte >> 7 == 0)
        return 1;
    else if (byte >> 6 == 2) //0b10 not first byte
        return -1;
    else if (byte >> 5 == 6) //0b110
        return 2;
    else if (byte >> 4 == 14) //0b1110
        return 3;
    else if (byte >> 3 == 31) //0b11110
        return 4;
    else
        return 0;
}

// Converts next utf8 character bytes to codepoint.
// Returns 0 on fail.
// TODO: test utf8_to_codepoint, it's currently untested
static uint32_t utf8_to_codepoint(unsigned char* bytes)
{
    if (bytes == NULL)
        return 0;

    int width = utf8_amount_of_bytes(bytes[0]);

    if (width <= 0)
        return 0;

    //ascii char
    if (width == 1)
        return (uint32_t)bytes[0];

    uint32_t codepoint = 0;

    //for the first byte, we only care about the bits after the first 0-bit, 
    //since the ones in front of it determines the width of utf8 character

    //add (8 - width (1-bits) - 1 (0-bit)) least significant bits to codepoint
    codepoint += bytes[0] & ((2 << (7 - width)) - 1);
    //shift over for next bits
    codepoint <<= 7 - width;

    //for remaining bytes except the last one, get the 6 least significant bits (skip the first 1 bit & 0 bit)
    //and shift the bits over unless we're at the last
    for (int i = 1; i < width - 1; i++)
    {
        codepoint += bytes[i] & ((2 << 6) - 1);
        codepoint <<= 6;
    }

    //for the last byte just add the 6 least significant bits
    codepoint += bytes[width - 1] & ((2 << 6) - 1);;

    return codepoint;
}

#pragma endregion

#pragma region Printing

//TODO: print json object

// Prints a ASCII character inside a string value (escaped if needed) into print buffer.
// Returns true on success, and false on fail.
static bool print_formatted_string_ascii_char(struct print_buffer* buffer, char character)
{
    if (buffer == NULL)
        return false;

    switch(character)
    {
        case '\"': //double quotation marks
            return print_buffer_append_string(buffer, "\\n");
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
            return print_buffer_append_char(buffer, character);
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
        //TODO: utf8 escaping

        if (!print_formatted_string_ascii_char(buffer, string[pos]))
            return false;

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
