#include "ki_json/json_generator.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include "ki_json/json.h"

struct print_buffer
{
    char* bytes;
    size_t buffer_size;
    size_t offset;
};

#pragma region Strings

// Returns length of string with max length of maxlen.
// This should be safer.
// C11 has strnlen_s & POSIX has strnlen, but this is C99.
static size_t ki_strnlen(const char* string, size_t maxlen)
{
    if (string == NULL)
        return 0;

    size_t length = 0;

    //increment until null-terminator or maxlen is reached
    while (string[length] != '\0' && length != maxlen)
        length++;

    return length;
}

#pragma region Buffer

// Doubles buffer size.
// Returns true on success, and false on fail.
static bool buffer_expand(struct print_buffer* buffer)
{
    if (buffer == NULL)
        return false;

    char* new_bytes = realloc(buffer->bytes, sizeof(char) * buffer->buffer_size * 2);

    //alloc fail
    if (new_bytes == NULL)
        return false;

    //fill new spots with null character
    for (size_t i = buffer->buffer_size; i < buffer->buffer_size * 2; i++)
        new_bytes[i] = '\0';

    buffer->bytes = new_bytes;
    buffer->buffer_size *= 2;

    return true;
}

static bool buffer_can_access(struct print_buffer* buffer, size_t pos)
{
    if (buffer == NULL)
        return false;

    return buffer->offset + pos < buffer->buffer_size;
}

static bool buffer_set_at(struct print_buffer* buffer, size_t pos, char byte)
{
    if (buffer == NULL)
        return false;

    if (!buffer_can_access(buffer, pos))
        return false;

    buffer->bytes[buffer->offset + pos] = byte;
    return true;
}

static char* buffer_buffer_at(struct print_buffer* buffer, size_t pos)
{
    if (buffer == NULL)
        return NULL;

    if (!buffer_can_access(buffer, pos))
        return NULL;

    return buffer->bytes + buffer->offset + pos;
}

#pragma endregion

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
// TODO: should probably take a buffer size for safety
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

//TODO: print json array
//TODO: print json object
//TODO: print json value

// Prints a single character into print buffer.
// Returns true on success, and false on fail.
static bool print_char(struct print_buffer* buffer, char character)
{
    if (buffer == NULL)
        return false;

    //if need to expand buffer, but failed to do so: return false
    if (!buffer_can_access(buffer, 0) && !buffer_expand(buffer))
        return false;

    if (!buffer_set_at(buffer, 0, character))
        return false;

    buffer->offset++;
    return true;
}

// Prints unformatted string into print buffer.
// Returns true on success, and false on fail.
static bool print_string(struct print_buffer* buffer, const char* string, size_t length)
{
    if (buffer == NULL || string == NULL)
        return false;

    //keep expanding buffer until we can print the given string
    while (!buffer_can_access(buffer, length - 1))
    {
        //failed to expand buffer
        if (!buffer_expand(buffer))
            return false;
    }

    for (size_t i = 0; i < length; i++)
        buffer_set_at(buffer, i, string[i]);

    buffer->offset += length;

    return true;
}

// Prints a ASCII character inside a string value (escaped if needed) into print buffer.
// Returns true on success, and false on fail.
static bool print_formatted_string_ascii_char(struct print_buffer* buffer, char character)
{
    if (buffer == NULL)
        return false;

    switch(character)
    {
        case '\"': //double quotation marks
            return print_string(buffer, "\\n", 2);
        case '\\': //reverse solidus
            return print_string(buffer, "\\\\", 2);
        case '\b': //backspace
            return print_string(buffer, "\\b", 2);
        case '\f': //form feed
            return print_string(buffer, "\\f", 2);
        case '\n': //line feed, line break
            return print_string(buffer, "\\n", 2);
        case '\r': //carriage return
            return print_string(buffer, "\\r", 2);
        case '\t': //horizontal tab
            return print_string(buffer, "\\t", 2);
        default:
            return print_char(buffer, character);
    }
}

// Prints json-formatted string into print buffer.
// Returns true on success, and false on fail.
static bool print_formatted_string(struct print_buffer* buffer, struct ki_string* string)
{
    if (buffer == NULL || string == NULL)
        return false;

    //start double quote
    if (!print_char(buffer, '\"'))
        return false;

    size_t length = ki_strnlen(string->bytes, string->buffer_size - 1);
    size_t pos = 0;
    
    while (pos < length)
    {
        char character = string->bytes[pos];

        //TODO: utf8 escaping

        if (!print_formatted_string_ascii_char(buffer, character))
            return false;

        pos++;
    }

    //end double quote
    if (!print_char(buffer, '\"'))
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
    size_t length = snprintf(number_string, NUM_MAX_LENGTH - 1, "%e", number);

    if (length == 0 || length >= NUM_MAX_LENGTH)
        return false;

    return print_string(buffer, number_string, length);
}

// Prints boolean into print buffer.
// Returns true on success, and false on fail.
static bool print_boolean(struct print_buffer* buffer, bool boolean)
{
    if (buffer == NULL)
        return false;

    if (boolean)
        return print_string(buffer, "true", 4);
    else 
        return print_string(buffer, "false", 5);
}

// Prints null (n-u-l-l, not the NULL character) into print buffer.
// Returns true on success, and false on fail.
static bool print_null(struct print_buffer* buffer)
{
    if (buffer == NULL)
        return false;

    return print_string(buffer, "null", 4);
}

// Prints json value into print buffer.
// Returns true on success, and false on fail.
static bool print_value(struct print_buffer* buffer, struct ki_json_val* val)
{
    if (buffer == NULL || val == NULL)
        return false;

    switch(val->type)
    {
        case KI_JSON_VAL_STRING:
            return print_formatted_string(buffer, &val->value.string);
        case KI_JSON_VAL_NUMBER:
            return print_number(buffer, val->value.number);
        case KI_JSON_VAL_BOOL:
            return print_boolean(buffer, val->value.boolean);
        case KI_JSON_VAL_NULL:
            return print_null(buffer);
        //TODO: case KI_JSON_VAL_ARRAY:
        //TODO: case KI_JSON_VAL_OBJECT:
        default:
            return false;
    }
}