#include "ki_json/json.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// Frees data of pair at index.
// NOTE: data of pair at index should be overwritten after calling this function.
// Returns true on success, false on fail.
static bool ki_json_object_free_pair_index(struct ki_json_object* object, size_t index)
{
    if (index >= object->count)
        return false;

    free(object->names[index]);
    ki_json_val_free(object->values[index]);
    object->names[index] = NULL;
    object->values[index] = NULL;

    return true;
}

bool ki_json_object_init(struct ki_json_object* object, size_t capacity)
{
    assert(object);

    object->capacity = capacity;
    object->count = 0;

    object->names = calloc(object->capacity, sizeof(*object->names));

    if (object->names == NULL)
    {
        free(object);
        return false;
    }

    object->values = calloc(object->capacity, sizeof(*object->values));

    if (object->values == NULL)
    {
        free(object->names);
        free(object);
        return false;
    }
    
    return true;
}

void ki_json_object_fini(struct ki_json_object* object)
{
    assert(object);
    
    for (size_t i = 0; i < object->count; i++)
        ki_json_object_free_pair_index(object, i);

    object->count = 0;
    object->capacity = 0;

    if (object->names != NULL)
    {
        free(object->names);
        object->names = NULL;
    }
    
    if (object->values != NULL)
    {
        free(object->values);
        object->values = NULL;
    }
}

/* Getting values */

// Returns val with given name in json object.
// Returns NULL on fail.
struct ki_json_val* ki_json_object_get(struct ki_json_object* object, const char* name)
{
    assert(object && name);

    for (size_t i = 0; i < object->count; i++)
    {
        if (strcmp(object->names[i], name) == 0)
            return object->values[i];
    }

    return NULL;
}

// Returns json object with given name in json object.
// Returns NULL on fail.
struct ki_json_object* ki_json_object_get_object(struct ki_json_object* object, const char* name)
{
    assert(object && name);

    struct ki_json_val* val = ki_json_object_get(object, name);

    if (val == NULL || val->type != KI_JSON_VAL_OBJECT)
        return NULL;

    return &val->value.object;
}

// Returns json array with given name in json object.
// Returns NULL on fail.
struct ki_json_array* ki_json_object_get_array(struct ki_json_object* object, const char* name)
{
    assert(object && name);

    struct ki_json_val* val = ki_json_object_get(object, name);

    if (val == NULL || val->type != KI_JSON_VAL_ARRAY)
        return NULL;

    return &val->value.array;
}

// Returns string with given name in json object.
// Returns NULL on fail.
char* ki_json_object_get_string(struct ki_json_object* object, const char* name)
{
    assert(object && name);

    struct ki_json_val* val = ki_json_object_get(object, name);

    if (val == NULL || val->type != KI_JSON_VAL_STRING)
        return NULL;

    return val->value.string;
}

// TODO: what to do on fail? ki_json_object_get_number
// Returns number with given name in json object.
double ki_json_object_get_number(struct ki_json_object* object, const char* name)
{
    assert(object && name);

    struct ki_json_val* val = ki_json_object_get(object, name);

    if (val == NULL || val->type != KI_JSON_VAL_NUMBER)
        return 0.0;

    return val->value.number;
}

// TODO: what to do on fail? ki_json_object_get_bool
// Returns bool with given name in json object.
bool ki_json_object_get_bool(struct ki_json_object* object, const char* name)
{
    assert(object && name);

    struct ki_json_val* val = ki_json_object_get(object, name);

    if (val == NULL || val->type != KI_JSON_VAL_BOOL)
        return false;

    return val->value.boolean;
}

/* Adding values */

// Doubles capacity of json object.
static bool ki_json_object_expand(struct ki_json_object* object)
{
    // allocate new arrays

    char** new_names = realloc(object->names, sizeof(*new_names) * object->capacity * 2);

    if (object->names == NULL)
        return false;

    struct ki_json_val** new_values = realloc(object->values, sizeof(*new_values) * object->capacity * 2);

    if (new_values == NULL)
    {
        free(new_names);
        return false;
    }

    // fill new spots with NULL
    for (size_t i = object->capacity; i < object->capacity * 2; i++)
    {
        new_names[i] = NULL;
        new_values[i] = NULL;
    }

    // set to new

    object->names = new_names;
    object->values = new_values;
    
    object->capacity *= 2;

    return true;
}

// Adds json value to json object as given name.
// NOTE 1: Ownership of value is given to json object, and will free it once done.
// NOTE 2: Name is copied.
enum ki_json_err_type ki_json_object_add(struct ki_json_object* object, const char* name, struct ki_json_val* value)
{
    //check if name already exists
    if (ki_json_object_get(object, name) != NULL)
        return KI_JSON_ERR_NAME_ALREADY_EXISTS;

    //if need to expand, but failed to do so
    if (object->count == object->capacity && !ki_json_object_expand(object))
        return KI_JSON_ERR_MEMORY;

    //copy name into our own allocated space so we can free it once we're done
    //FIXME: this has no limit on how much it can copy
    
    size_t name_length = strlen(name);
    char* copy = calloc(name_length + 1, sizeof(*copy));

    if (copy == NULL)
        return KI_JSON_ERR_MEMORY;

    strncpy(copy, name, name_length);

    copy[name_length] = '\0'; //null-terminator

    object->names[object->count] = copy;
    object->values[object->count] = value; 
    object->count++;

    return KI_JSON_ERR_NONE;
}

// Creates new json value for a json object and adds it to the json object.
// NOTE: Name is copied.
// Returns NULL on fail.
struct ki_json_val* ki_json_object_add_new_object(struct ki_json_object* object, const char* name, size_t capacity)
{
    struct ki_json_val* val = ki_json_val_create_object(capacity);

    if (val == NULL)
        return NULL;

    if (ki_json_object_add(object, name, val) != KI_JSON_ERR_NONE)
    {
        ki_json_val_free(val);
        val = NULL;
    }

    return val;

}
// Creates new json value for a json array and adds it to the json object.
// NOTE: Name is copied.
// Returns NULL on fail.
struct ki_json_val* ki_json_object_add_new_array(struct ki_json_object* object, const char* name, size_t capacity)
{
    struct ki_json_val* val = ki_json_val_create_array(capacity);

    if (val == NULL)
        return NULL;

    if (ki_json_object_add(object, name, val) != KI_JSON_ERR_NONE)
    {
        ki_json_val_free(val);
        val = NULL;
    }

    return val;
}

// Creates new json value for a string and adds it to the json object.
// NOTE: Name & string is copied.
// Returns NULL on fail.
struct ki_json_val* ki_json_object_add_new_string(struct ki_json_object* object, const char* name, const char* string)
{
    struct ki_json_val* val = ki_json_val_create_from_string(string);

    if (val == NULL)
        return NULL;

    if (ki_json_object_add(object, name, val) != KI_JSON_ERR_NONE)
    {
        ki_json_val_free(val);
        val = NULL;
    }

    return val;
}

// Creates new json value for a number and adds it to the json object.
// NOTE: Name is copied.
// Returns NULL on fail.
struct ki_json_val* ki_json_object_add_new_number(struct ki_json_object* object, const char* name, double number)
{
    struct ki_json_val* val = ki_json_val_create_from_number(number);

    if (val == NULL)
        return NULL;

    if (ki_json_object_add(object, name, val) != KI_JSON_ERR_NONE)
    {
        ki_json_val_free(val);
        val = NULL;
    }

    return val;
}

// Creates new json value for a bool and adds it to the json object.
// NOTE: Name is copied.
// Returns NULL on fail.
struct ki_json_val* ki_json_object_add_new_bool(struct ki_json_object* object, const char* name, bool boolean)
{
    struct ki_json_val* val = ki_json_val_create_from_bool(boolean);

    if (val == NULL)
        return NULL;

    if (ki_json_object_add(object, name, val) != KI_JSON_ERR_NONE)
    {
        ki_json_val_free(val);
        val = NULL;
    }

    return val;
}

// Creates new json value representing null and adds it to the json object.
// NOTE: Name is copied.
// Returns NULL on fail.
struct ki_json_val* ki_json_object_add_new_null(struct ki_json_object* object, const char* name)
{
    struct ki_json_val* val = ki_json_val_create_null();

    if (val == NULL)
        return NULL;

    if (ki_json_object_add(object, name, val) != KI_JSON_ERR_NONE)
    {
        ki_json_val_free(val);
        val = NULL;
    }

    return val;
}

/* Setting values */

// NOTE 1: Value must be of type KI_JSON_VAL_STRING.
// NOTE 2: String is copied.
// Returns true on success, false on fail.
bool ki_json_object_set_string(struct ki_json_object* object, const char* name, const char* string)
{
    struct ki_json_val* val = ki_json_object_get(object, name);

    if (val == NULL)
        return false;

    return ki_json_val_set_string(val, string);
}

// NOTE: Value must be of type KI_JSON_VAL_NUMBER.
// Returns true on success, false on fail.
bool ki_json_object_set_number(struct ki_json_object* object, const char* name, double number)
{
    struct ki_json_val* val = ki_json_object_get(object, name);

    if (val == NULL || val->type != KI_JSON_VAL_NUMBER)
        return false;

    val->value.number = number;

    return true;
}

// NOTE: Value must be of type KI_JSON_VAL_BOOL.
// Returns true on success, false on fail.
bool ki_json_object_set_bool(struct ki_json_object* object, const char* name, bool boolean)
{
    struct ki_json_val* val = ki_json_object_get(object, name);

    if (val == NULL || val->type != KI_JSON_VAL_BOOL)
        return false;

    val->value.boolean = boolean;

    return true;
}

/* Removing values */

bool ki_json_object_remove(struct ki_json_object* object, const char* name)
{
    //find index of pair with name

    int index = -1;
    
    for (size_t i = 0; i < object->count; i++)
    {
        if (strcmp(object->names[i], name) == 0)
        {
            index = i;
            break;
        }
    }

    //failed to find pair with name
    if (index == -1)
        return false;

    //free found pair
    ki_json_object_free_pair_index(object, index);

    //move pairs to right of it back one space, overwriting index with freed pair

    for (size_t i = index + 1; i < object->count; i++)
    {
        object->names[i - 1] = object->names[i];
        object->values[i - 1] = object->values[i];    
    }

    object->names[object->count - 1] = NULL;
    object->values[object->count - 1] = NULL;
    object->count--;

    return false;
}

