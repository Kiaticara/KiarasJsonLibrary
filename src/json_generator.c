#include "ki_json/json_generator.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>

#include "ki_json/json.h"

struct print_buffer
{
    char* bytes;
    size_t buffer_size;
    size_t offset;
};

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

#pragma region Conversions

//TODO: utf8_to_unicode_codepoint
//TODO: unicode_codepoint_to_str

//TODO: escape sequences

#pragma endregion

#pragma region Printing

//TODO: print number
//TODO: print boolean
//TODO: print null
//TODO: print json-formatted string
//TODO: print json array
//TODO: print json object
//TODO: print json value

// Prints unformatted string into print buffer.
// Returns true on success, and false on fail.
static bool print_string(struct print_buffer* buffer, const char* string, size_t length)
{
    if (buffer == NULL || string == NULL)
        return false;

    if (!buffer_can_access(buffer, length - 1))
        return false;

    for (size_t i = 0; i < length; i++)
        buffer_set_at(buffer, i, string[i]);

    buffer->offset += length;

    return true;
}

