#ifndef KI_JSON_PARSER_H
#define KI_JSON_PARSER_H

// Functions for parsing a json string to a json tree

#include <stddef.h>

#include "ki_json/json.h"

// Parse null-terminated string to a json tree.
// Returns NULL on fail.
struct ki_json_val* ki_json_parse_string(const char* string);

// Parse no more than n characters of string to a json tree.
// Returns NULL on fail.
struct ki_json_val* ki_json_nparse_string(const char* string, size_t n);

#endif //KI_JSON_PARSER_H
