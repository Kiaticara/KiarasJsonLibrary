#ifndef KI_JSON_PARSER_H
#define KI_JSON_PARSER_H

// Functions for parsing a json string to a json tree

#include <stddef.h>

#include "ki_json/json.h"

struct ki_json_parser_err
{
    enum ki_json_err_type type;
    const char* json;
    size_t pos;
};

// Parse null-terminated string to a json tree.
// Val returned must be freed using ki_json_val_free() when done.
// Returns NULL on fail and outs error to err.
struct ki_json_val* ki_json_parse_string(const char* string, struct ki_json_parser_err* err);

// Parse no more than n characters of string to a json tree.
// Val returned must be freed using ki_json_val_free() when done.
// Returns NULL on fail and outs error to err.
struct ki_json_val* ki_json_nparse_string(const char* string, size_t n, struct ki_json_parser_err* err);

#endif //KI_JSON_PARSER_H
