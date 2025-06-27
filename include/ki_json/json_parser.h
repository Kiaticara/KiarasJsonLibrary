#ifndef KI_JSON_PARSER_H
#define KI_JSON_PARSER_H

// Functions for parsing a json string to a json tree

#include <stddef.h>

#include "ki_json/json.h"

enum ki_json_parser_error_type
{
    KI_JSON_PARSER_ERROR_NONE, //no error.
    KI_JSON_PARSER_ERROR_UNKNOWN, //uh oh...

    KI_JSON_PARSER_ERROR_MEMORY, //memory error, for ex.: allocation fail

    KI_JSON_PARSER_ERROR_UNTERMINATED_STRING, //string value does not end
    KI_JSON_PARSER_ERROR_UNTERMINATED_ARRAY, //array does not end
    KI_JSON_PARSER_ERROR_UNTERMINATED_OBJECT, //object does not end

    KI_JSON_PARSER_ERROR_NAME_ALREADY_EXISTS, //name already exists in object
    KI_JSON_PARSER_ERROR_NO_NAME_VALUE_SEPARATOR, //separator : does not exist between name & value
    KI_JSON_PARSER_ERROR_NO_VALUE, //name of name-value pair in object does have a value

    KI_JSON_PARSER_ERROR_UNKNOWN_TOKEN, //parser can't resolve type of token (number, string, object, array, null)
    KI_JSON_PARSER_ERROR_UNKNOWN_ESCAPE_SEQUENCE,

    KI_JSON_PARSER_ERROR_TRAILING_COMMA, //Trailing comma in array or object is not supported.

    KI_JSON_PARSER_ERROR_COPY_FAIL, //Should be reported if occurs.
};

// Parse null-terminated string to a json tree.
// Returns NULL on fail.
struct ki_json_val* ki_json_parse_string(const char* string);

// Parse no more than n characters of string to a json tree.
// Returns NULL on fail.
struct ki_json_val* ki_json_nparse_string(const char* string, size_t n);

#endif //KI_JSON_PARSER_H
