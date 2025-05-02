#include "print_buffer.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// Init print buffer with given starting size.
// Returns true on success, false on fail.
bool print_buffer_init(struct print_buffer* buffer, size_t size)
{
    if (buffer == NULL || size <= 0)
        return false;

    buffer->bytes = calloc(size, sizeof(*buffer->bytes));

    if (buffer->bytes == NULL)
        return false;

    buffer->size = size;
    buffer->pos = 0;
    buffer->bytes[buffer->pos] = '\0';

    return true;
}

void print_buffer_fini(struct print_buffer* buffer)
{
    if (buffer == NULL)
        return;

    if (buffer->bytes != NULL)
    {
        free(buffer->bytes);
        buffer->bytes = NULL;
    }

    buffer->size = 0;
    buffer->pos = 0;
}

// Sets position of print buffer to 0, and starts a new string.
void print_buffer_reset(struct print_buffer* buffer)
{
    if (buffer == NULL)
        return;

    buffer->pos = 0;
    buffer->bytes[buffer->pos] = '\0';
}

// Returns length in bytes of current string inside print buffer.
size_t print_buffer_length(struct print_buffer* buffer)
{
    return (buffer != NULL) ? buffer->pos : 0;
}

// Possibly reallocs the buffer to ensure that the print buffer has enough space for SIZE bytes.
// Returns true on success, and false on fail.
bool print_buffer_ensure_size(struct print_buffer* buffer, size_t size)
{
    if (buffer == NULL)
        return false;

    //uninit print buffer...
    if (buffer->size == 0)
        return false;

    //already have enough space, don't need to do anything
    if (buffer->size >= size)
        return true;

    size_t new_size = buffer->size;
    while (new_size < size)
        new_size *= 2;

    unsigned char* new_bytes = realloc(buffer->bytes, sizeof(*new_bytes) * new_size);

    //alloc fail
    if (new_bytes == NULL)
        return false;

    //fill new spots with null character
    for (size_t i = buffer->size; i < new_size; i++)
        new_bytes[i] = '\0';

    buffer->bytes = new_bytes;
    buffer->size = new_size;

    return true;
}

// Adds a single character to the end of print buffer.
// Returns true on success, and false on fail.
bool print_buffer_append_char(struct print_buffer* buffer, char character)
{
    if (buffer == NULL)
        return false;

    if (!print_buffer_ensure_size(buffer, buffer->pos + 2))
        return false;

    buffer->bytes[buffer->pos] = (unsigned char)character;
    buffer->pos++;
    buffer->bytes[buffer->pos] = '\0';

    return true;
}

// Adds memory of given size to the end of print buffer.
// Returns true on success, and false on fail.
bool print_buffer_append_mem(struct print_buffer* buffer, const unsigned char* mem, size_t size)
{
    if (buffer == NULL || mem == NULL)
        return false;

    if (!print_buffer_ensure_size(buffer, buffer->pos + size + 1))
        return false;

    for (size_t i = 0; i < size; i++)
    {
        buffer->bytes[buffer->pos] = mem[i];
        buffer->pos++;
    }

    buffer->bytes[buffer->pos] = '\0';
    return true;
}

// Adds string to the end of print buffer.
// Returns true on success, and false on fail.
bool print_buffer_append_string(struct print_buffer* buffer, const char* string)
{
    if (buffer == NULL || string == NULL)
        return false;

    size_t length = strlen(string);

    if (!print_buffer_ensure_size(buffer, buffer->pos + length + 1))
        return false;

    for (size_t i = 0; i < length; i++)
    {
        buffer->bytes[buffer->pos] = (unsigned char)string[i];
        buffer->pos++;
    }

    buffer->bytes[buffer->pos] = '\0';
    return true;
}

// Truncates if necessary.
// Returns whether src was completely copied over to dest.
bool print_buffer_copy_to_buffer(struct print_buffer* src, char* dest, size_t size)
{
    if (src == NULL || dest == NULL)
        return false;

    if (size <= 0)
        return false;

    size_t src_length = print_buffer_length(src);

    //MIN(size - 1, src_length)
    size_t copy_length = (size - 1 > src_length) ? src_length : size - 1;

    for (size_t i = 0; i < copy_length; i++)
        dest[i] = src->bytes[i];

    dest[copy_length] = '\0';
    return copy_length == src_length;
}
