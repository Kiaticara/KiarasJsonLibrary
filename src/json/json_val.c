#include "ki_json/json.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* Creating */

// Creates a json value for a json object with given starting capacity.
// Returns NULL on fail.
struct ki_json_val* ki_json_val_create_object(size_t capacity)
{
    struct ki_json_val* val = calloc(1, sizeof(*val));

    if (val == NULL)
        return NULL;

    val->type = KI_JSON_VAL_OBJECT;
    ki_json_object_init(&val->value.object, capacity);

    return val;
}

// Creates a json value for a json array with given starting capacity.
// Returns NULL on fail.
struct ki_json_val* ki_json_val_create_array(size_t capacity)
{
    struct ki_json_val* val = calloc(1, sizeof(*val));

    if (val == NULL)
        return NULL;

    val->type = KI_JSON_VAL_ARRAY;
    ki_json_array_init(&val->value.array, capacity);

    return val;
}

// Creates a json value from a string.
// NOTE: String is copied.
// Returns NULL on fail.
struct ki_json_val* ki_json_val_create_from_string(const char* string)
{
    struct ki_json_val* val = calloc(1, sizeof(*val));

    if (val == NULL)
        return NULL;

    val->type = KI_JSON_VAL_STRING;
    val->value.string = NULL;
    ki_json_val_set_string(val, string);

    return val;
}

// Creates a json value from a double.
// Returns NULL on fail.
struct ki_json_val* ki_json_val_create_from_number(double number)
{
    struct ki_json_val* val = calloc(1, sizeof(*val));

    if (val == NULL)
        return NULL;

    val->type = KI_JSON_VAL_NUMBER;
    val->value.number = number;

    return val;
}

// Creates a json value from a bool.
// Returns NULL on fail.
struct ki_json_val* ki_json_val_create_from_bool(bool boolean)
{
    struct ki_json_val* val = calloc(1, sizeof(*val));

    if (val == NULL)
        return NULL;
    
    val->type = KI_JSON_VAL_BOOL;
    val->value.boolean = boolean;

    return val;
}

// Creates a json value representing null.
// Returns NULL on fail.
struct ki_json_val* ki_json_val_create_null(void)
{
    struct ki_json_val* val = calloc(1, sizeof(*val));

    if (val == NULL)
        return NULL;

    val->type = KI_JSON_VAL_NULL;
    val->value.null = true;

    return val;
}

/* Special setters */

// NOTE 1: ki_json_value must be of type KI_JSON_VAL_STRING.
// NOTE 2: String is copied.
// Returns true on success, false on fail.
bool ki_json_val_set_string(struct ki_json_val* val, const char* string)
{
    assert(val && string && val->type == KI_JSON_VAL_STRING);

    //copy string into our own allocated space so we can free it once we're done
    //FIXME: this has no limit on how much it can copy
    
    size_t length = strlen(string);
    char* copy = calloc(length + 1, sizeof(*(val->value.string)));

    if (copy == NULL)
        return false;

    strncpy(copy, string, length);
    copy[length] = '\0'; //null-terminator

    //free old string
    if (val->value.string != NULL)
        free(val->value.string);

    val->value.string = copy;

    return true;
}

/* Freeing */

void ki_json_val_free(struct ki_json_val* val)
{
    if (val == NULL)
        return;

    switch (val->type)
    {
        case KI_JSON_VAL_OBJECT:
            ki_json_object_fini(&val->value.object);
            break;
        case KI_JSON_VAL_ARRAY:
            ki_json_array_fini(&val->value.array);
            break;
        case KI_JSON_VAL_STRING:
            if (val->value.string != NULL)
            {
                free(val->value.string);
                val->value.string = NULL;
            }
            break;
        default: //KI_JSON_VAL_BOOL, KI_JSON_VAL_NUMBER, KI_JSON_VAL_NULL
            break;
    }

    free(val);
}

