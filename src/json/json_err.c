#include "ki_json/json.h"

static const char* err_msgs[] = {
    [KI_JSON_ERR_NONE] = "No error occurred.",
    [KI_JSON_ERR_INTERNAL] = "An internal error occurred.",
    [KI_JSON_ERR_TOO_SHORT] = "Json string was expected to be longer.",
    [KI_JSON_ERR_INVALID_ARGS] = "User gave invalid arguments.",
    [KI_JSON_ERR_MEMORY] = "A memory error occurred.",
    [KI_JSON_ERR_UNTERMINATED_STRING] = "Expected ending '\"' to terminate string.",
    [KI_JSON_ERR_UNTERMINATED_ARRAY] = "Expected ending ']' to terminate array.",
    [KI_JSON_ERR_UNTERMINATED_OBJECT] = "Expected ending '}' to terminate object.",
    [KI_JSON_ERR_EXPECTED_NAME] = "Expected string as name for name-value pair.",
    [KI_JSON_ERR_NAME_ALREADY_EXISTS] = "Duplicate pair name inside object.",
    [KI_JSON_ERR_EXPECTED_NAME_VALUE_SEPARATOR] = "Expected ':' to separate name and value.",
    [KI_JSON_ERR_UNKNOWN_TOKEN] = "Unable to resolve json token.",
    [KI_JSON_ERR_INVALID_ESCAPE_SEQUENCE] = "Invalid escape sequence.",
    [KI_JSON_ERR_TRAILING_COMMA] = "Trailing commas are not allowed."
};

// Get error message for json error type.
// Returns NULL for nonexistent error types.
const char* ki_json_err_get_message(enum ki_json_err_type err)
{
    if (err >= 0 && err <= KI_JSON_ERR_AMOUNT)
        return err_msgs[err];
    else
        return NULL;
}
