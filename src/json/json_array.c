#include "ki_json/json.h"

#include <stddef.h>
#include <stdlib.h>
#include <assert.h>

bool ki_json_array_init(struct ki_json_array* array, size_t capacity)
{
    assert(array);

    array->capacity = capacity;
    array->count = 0;

    array->values = calloc(array->capacity, sizeof(*array->values));

    if (array->values == NULL)
    {
        free(array);
        return false;
    }
    
    return true;
}

void ki_json_array_fini(struct ki_json_array* array)
{
    assert(array);

    if (array->values != NULL)
    {
        for (size_t i = 0; i < array->count; i++)
            ki_json_val_free(array->values[i]);

        free(array->values);
        array->values = NULL;
    }

    array->count = 0;
    array->capacity = 0;
}

#pragma region Getting values

struct ki_json_val* ki_json_array_at(struct ki_json_array* array, size_t index)
{
    assert(array);

    if (index < 0 || index >= array->count)
        return NULL;

    return array->values[index];
}

// Returns json object at given index in json array.
// Returns NULL on fail.
struct ki_json_object* ki_json_array_object_at(struct ki_json_array* array, size_t index)
{
    struct ki_json_val* val = ki_json_array_at(array, index);

    if (val == NULL || val->type != KI_JSON_VAL_OBJECT)
        return NULL;

    return &val->object;
}

// Returns json array at given index in json array.
// Returns NULL on fail.
struct ki_json_array* ki_json_array_array_at(struct ki_json_array* array, size_t index)
{
    struct ki_json_val* val = ki_json_array_at(array, index);

    if (val == NULL || val->type != KI_JSON_VAL_ARRAY)
        return NULL;

    return &val->array;
}

// Returns string at given index in json array.
// Returns NULL on fail.
struct ki_string* ki_json_array_string_at(struct ki_json_array* array, size_t index)
{
    struct ki_json_val* val = ki_json_array_at(array, index);

    if (val == NULL || val->type != KI_JSON_VAL_STRING)
        return NULL;

    return &val->string;
}

// TODO: what to do on fail? ki_json_array_get_number
// Returns number at given index in json array.
double ki_json_array_number_at(struct ki_json_array* array, size_t index)
{
    struct ki_json_val* val = ki_json_array_at(array, index);

    if (val == NULL || val->type != KI_JSON_VAL_NUMBER)
        return 0.0;

    return val->number;
}

// TODO: what to do on fail? ki_json_array_get_bool
// Returns bool at given index in json array.
bool ki_json_array_bool_at(struct ki_json_array* array, size_t index)
{
    struct ki_json_val* val = ki_json_array_at(array, index);

    if (val == NULL || val->type != KI_JSON_VAL_BOOL)
        return false;

    return val->boolean;
}

#pragma endregion

#pragma region Inserting values

// Doubles capacity of json array.
static bool ki_json_array_expand(struct ki_json_array* array)
{
    //allocate new arrays
    struct ki_json_val** new_values = realloc(array->values, sizeof(*new_values) * array->capacity * 2);

    if (new_values == NULL)
        return false;

    //fill new spots with NULL
    for (size_t i = array->capacity; i < array->capacity * 2; i++)
        new_values[i] = NULL;

    //set to new
    array->values = new_values;
    array->capacity *= 2;

    return true;
}

// Adds json value to json array at given index.
// NOTE: Ownership of value is given to json array, and will free it once done.
// Returns true on success, false on fail.
bool ki_json_array_insert(struct ki_json_array* array, struct ki_json_val* value, size_t index)
{
    //is index out-of-bounds? then return false
    //allow inserting at array->count => at end of json array
    if (index < 0 || index > array->count)
        return false;

    //if need to expand json array, but failed to do so, return false
    if (array->count == array->capacity && !ki_json_array_expand(array))
        return false;

    //shift all items starting at given index to the right by one space
    if (array->count != 0)
    {
        for (size_t i = array->count - 1; i >= index; i--)
            array->values[i + 1] = array->values[i];
    }

    array->values[index] = value;
    array->count++;

    return true;
}

// Creates new json value for a json object and adds it to the json array at given index.
// Returns NULL on fail.
struct ki_json_val* ki_json_array_insert_new_object(struct ki_json_array* array, size_t index, size_t capacity)
{
    struct ki_json_val* val = ki_json_val_create_object(capacity);

    if (val == NULL)
        return NULL;

    ki_json_array_insert(array, val, index);

    return val;
}

// Creates new json value for a json array and adds it to the json array at given index.
// Returns NULL on fail.
struct ki_json_val* ki_json_array_insert_new_array(struct ki_json_array* array, size_t index, size_t capacity)
{
    struct ki_json_val* val = ki_json_val_create_array(capacity);

    if (val == NULL)
        return NULL;

    ki_json_array_insert(array, val, index);

    return val;
}

// Creates new json value for a string and adds it to the json array at given index.
// NOTE: String is copied.
// Returns NULL on fail.
struct ki_json_val* ki_json_array_insert_new_string(struct ki_json_array* array, size_t index, const char* string)
{
    struct ki_json_val* val = ki_json_val_create_from_string(string);

    if (val == NULL)
        return NULL;

    ki_json_array_insert(array, val, index);

    return val;
}

// Creates new json value for a number and adds it to the json array at given index.
// Returns NULL on fail.
struct ki_json_val* ki_json_array_insert_new_number(struct ki_json_array* array, size_t index, double number)
{
    struct ki_json_val* val = ki_json_val_create_from_number(number);

    if (val == NULL)
        return NULL;

    ki_json_array_insert(array, val, index);

    return val;
}

// Creates new json value for a bool and adds it to the json array at given index.
// Returns NULL on fail.
struct ki_json_val* ki_json_array_insert_new_bool(struct ki_json_array* array, size_t index, bool boolean)
{
    struct ki_json_val* val = ki_json_val_create_from_bool(boolean);

    if (val == NULL)
        return NULL;

    ki_json_array_insert(array, val, index);

    return val;
}

// Creates new json value representing null and adds it to the json array at given index.
// Returns NULL on fail.
struct ki_json_val* ki_json_array_insert_new_null(struct ki_json_array* array, size_t index)
{
    struct ki_json_val* val = ki_json_val_create_null();

    if (val == NULL)
        return NULL;

    ki_json_array_insert(array, val, index);

    return val;
}


#pragma endregion

#pragma region Adding values

// Adds json value to the end of a json array.
// NOTE: Ownership of value is given to json array, and will free it once done.
// Returns true on success, false on fail.
bool ki_json_array_add(struct ki_json_array* array, struct ki_json_val* value)
{
    return ki_json_array_insert(array, value, array->count);
}

// Creates new json value for a json object and adds it to the end of a json array.
// Returns NULL on fail.
struct ki_json_val* ki_json_array_add_new_object(struct ki_json_array* array, size_t capacity)
{
    struct ki_json_val* val = ki_json_val_create_object(capacity);

    if (val == NULL)
        return NULL;

    ki_json_array_add(array, val);

    return val;
}

// Creates new json value for a json array and adds it to the end of a json array.
// Returns NULL on fail.
struct ki_json_val* ki_json_array_add_new_array(struct ki_json_array* array, size_t capacity)
{
    struct ki_json_val* val = ki_json_val_create_array(capacity);

    if (val == NULL)
        return NULL;

    ki_json_array_add(array, val);

    return val;
}

// Creates new json value for a string and adds it to the end of a json array.
// NOTE: String is copied.
// Returns NULL on fail.
struct ki_json_val* ki_json_array_add_new_string(struct ki_json_array* array, const char* string)
{
    struct ki_json_val* val = ki_json_val_create_from_string(string);

    if (val == NULL)
        return NULL;

    ki_json_array_add(array, val);

    return val;
}

// Creates new json value for a number and adds it to the end of a json array.
// Returns NULL on fail.
struct ki_json_val* ki_json_array_add_new_number(struct ki_json_array* array, double number)
{
    struct ki_json_val* val = ki_json_val_create_from_number(number);

    if (val == NULL)
        return NULL;

    ki_json_array_add(array, val);

    return val;
}

// Creates new json value for a bool and adds it to the end of a json array.
// Returns NULL on fail.
struct ki_json_val* ki_json_array_add_new_bool(struct ki_json_array* array, bool boolean)
{
    struct ki_json_val* val = ki_json_val_create_from_bool(boolean);

    if (val == NULL)
        return NULL;

    ki_json_array_add(array, val);

    return val;
}

// Creates new json value representing null and adds it to the end of a json array.
// Returns NULL on fail.
struct ki_json_val* ki_json_array_add_new_null(struct ki_json_array* array)
{
    struct ki_json_val* val = ki_json_val_create_null();

    if (val == NULL)
        return NULL;

    ki_json_array_add(array, val);

    return val;
}

#pragma endregion

#pragma region Setting values

// NOTE 1: Value must be of type KI_JSON_VAL_STRING.
// NOTE 2: String is copied.
// Returns true on success, false on fail.
bool ki_json_array_set_string(struct ki_json_array* array, size_t index, const char* string)
{
    struct ki_json_val* val = ki_json_array_at(array, index);

    if (val == NULL)
        return false;

    return ki_json_val_set_string(val, string);
}

// NOTE: Value must be of type KI_JSON_VAL_NUMBER.
// Returns true on success, false on fail.
bool ki_json_array_set_number(struct ki_json_array* array, size_t index, double number)
{
    struct ki_json_val* val = ki_json_array_at(array, index);

    if (val == NULL || val->type != KI_JSON_VAL_NUMBER)
        return false;

    val->number = number;

    return true;
}

// NOTE: Value must be of type KI_JSON_VAL_BOOL.
// Returns true on success, false on fail.
bool ki_json_array_set_bool(struct ki_json_array* array, size_t index, bool boolean)
{
    struct ki_json_val* val = ki_json_array_at(array, index);

    if (val == NULL || val->type != KI_JSON_VAL_BOOL)
        return false;

    val->boolean = boolean;

    return true;
}

#pragma endregion

#pragma region Removing values

// Returns true on success, false on fail.
bool ki_json_array_remove_at(struct ki_json_array* array, size_t index)
{
    if (index < 0 || index >= array->count)
        return false;

    ki_json_val_free(array->values[index]);

    //move values to right of it back one space, overwriting index with free'd value

    for (int i = index + 1; i < array->count; i++)
        array->values[i - 1] = array->values[i];    

    array->values[array->count - 1] = NULL;
    array->count--;

    return true;
}

// Removes first occurence of reference to given json value.
// Returns true on success, false on fail.
bool ki_json_array_remove(struct ki_json_array* array, struct ki_json_val* value)
{
    //find index of reference to value, and remove it at that index
    for (int i = 0; i < array->count; i++)
    {
        if (array->values[i] == value)
        {
            ki_json_array_remove_at(array, i);
            return true;
        }
    }

    //failed to find value
    return false;
}

#pragma endregion