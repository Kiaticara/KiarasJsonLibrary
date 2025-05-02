#pragma once

#include <stddef.h>
#include <stdbool.h>

// Internal ki_json: do not use outside

struct print_buffer
{
    unsigned char* bytes;
    size_t size;
    size_t pos;
};

// Init print buffer with given starting size.
// Returns true on success, false on fail.
bool print_buffer_init(struct print_buffer* buffer, size_t size);

void print_buffer_fini(struct print_buffer* buffer);

// Sets position of print buffer to 0, and starts a new string.
void print_buffer_reset(struct print_buffer* buffer);

// Returns length in bytes of current string inside print buffer.
size_t print_buffer_length(struct print_buffer* buffer);

// Possibly reallocs the buffer to ensure that the print buffer has enough space for SIZE bytes.
// Returns true on success, and false on fail.
bool print_buffer_ensure_size(struct print_buffer* buffer, size_t size);

// Adds a single character to the end of print buffer.
// Returns true on success, and false on fail.
bool print_buffer_append_char(struct print_buffer* buffer, char character);

// Adds memory of given size to the end of print buffer.
// Returns true on success, and false on fail.
bool print_buffer_append_mem(struct print_buffer* buffer, const unsigned char* mem, size_t size);

// Adds string to the end of print buffer.
// Returns true on success, and false on fail.
bool print_buffer_append_string(struct print_buffer* buffer, const char* string);

// Truncates if necessary.
// Returns whether src was completely copied over to dest.
bool print_buffer_copy_to_buffer(struct print_buffer* src, char* dest, size_t size);
