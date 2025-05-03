#pragma once

#include <stdbool.h>
#include <stddef.h>

// Contains what is needed to represent json trees, along with the functions neccessary for creating & interacting with them

#pragma region Structs & enums

enum KI_JSON_VAL_TYPE
{
    KI_JSON_VAL_NULL = 0, //nothing, NULL
    KI_JSON_VAL_OBJECT = 1, //object, struct ki_json_object
    KI_JSON_VAL_ARRAY = 2, //array, struct ki_json_array
    KI_JSON_VAL_STRING = 3, //string, char*
    KI_JSON_VAL_NUMBER = 4, //number, double
    KI_JSON_VAL_BOOL = 5 //boolean, bool
};

// A collection of json name/value pairs.
struct ki_json_object
{
    // Names of pairs
    char** names;
    // Values of pairs
    struct ki_json_val** values;
    // Number of pairs currently in this json object
    size_t count;
    // Maximum amount of pairs this json object can currently hold.
    // Expands automatically, will never shrink
    size_t capacity;
};

// An ordered list of values.
// Uses an array to hold the values.
struct ki_json_array
{
    struct ki_json_val** values;
    // Number of values currently in this json object
    size_t count;
    // Maximum amount of values this json object can currently hold.
    // Expands automatically, will never shrink
    size_t capacity;
};

// An json value.
// NOTE: strings should be set using ki_json_val_set_string
struct ki_json_val
{
    enum KI_JSON_VAL_TYPE type;
    
    union
    {
        struct ki_json_object object;
        struct ki_json_array array;
        char* string;
        double number;
        bool boolean;
        bool null; 
    } value;
};

#pragma endregion

//TODO: add val, object and array is_type functions

#pragma region Json object functions

bool ki_json_object_init(struct ki_json_object* object, size_t capacity);
void ki_json_object_fini(struct ki_json_object* object);

// Returns val with given name in json object.
// Returns NULL on fail.
struct ki_json_val* ki_json_object_get(struct ki_json_object* object, const char* name);
// Returns json object with given name in json object.
// Returns NULL on fail.
struct ki_json_object* ki_json_object_get_object(struct ki_json_object* object, const char* name);
// Returns json array with given name in json object.
// Returns NULL on fail.
struct ki_json_array* ki_json_object_get_array(struct ki_json_object* object, const char* name);
// Returns string with given name in json object.
// Returns NULL on fail.
char* ki_json_object_get_string(struct ki_json_object* object, const char* name);
// TODO: what to do on fail? ki_json_object_get_number
// Returns number with given name in json object.
// NOTE: Returns 0.0 on fail.
double ki_json_object_get_number(struct ki_json_object* object, const char* name);
// TODO: what to do on fail? ki_json_object_get_bool
// Returns bool with given name in json object.
// NOTE: Returns false on fail.
bool ki_json_object_get_bool(struct ki_json_object* object, const char* name);

// Adds json value to json object as given name.
// NOTE 1: Ownership of value is given to json object, and will free it once done.
// NOTE 2: Name is copied.
// Returns true on success, false on fail.
bool ki_json_object_add(struct ki_json_object* object, const char* name, struct ki_json_val* value);
// Creates new json value for a json object and adds it to the json object.
// NOTE: Name is copied.
// Returns NULL on fail.
struct ki_json_val* ki_json_object_add_new_object(struct ki_json_object* object, const char* name, size_t capacity);
// Creates new json value for a json array and adds it to the json object.
// NOTE: Name is copied.
// Returns NULL on fail.
struct ki_json_val* ki_json_object_add_new_array(struct ki_json_object* object, const char* name, size_t capacity);
// Creates new json value for a string and adds it to the json object.
// NOTE: Name & string is copied.
// Returns NULL on fail.
struct ki_json_val* ki_json_object_add_new_string(struct ki_json_object* object, const char* name, const char* string);
// Creates new json value for a number and adds it to the json object.
// NOTE: Name is copied.
// Returns NULL on fail.
struct ki_json_val* ki_json_object_add_new_number(struct ki_json_object* object, const char* name, double number);
// Creates new json value for a bool and adds it to the json object.
// NOTE: Name is copied.
// Returns NULL on fail.
struct ki_json_val* ki_json_object_add_new_bool(struct ki_json_object* object, const char* name, bool boolean);
// Creates new json value representing null and adds it to the json object.
// NOTE: Name is copied.
// Returns NULL on fail.
struct ki_json_val* ki_json_object_add_new_null(struct ki_json_object* object, const char* name);

// NOTE 1: Value must be of type KI_JSON_VAL_STRING.
// NOTE 2: String is copied.
// Returns true on success, false on fail.
bool ki_json_object_set_string(struct ki_json_object* object, const char* name, const char* string);
// NOTE: Value must be of type KI_JSON_VAL_NUMBER.
// Returns true on success, false on fail.
bool ki_json_object_set_number(struct ki_json_object* object, const char* name, double number);
// NOTE: Value must be of type KI_JSON_VAL_BOOL.
// Returns true on success, false on fail.
bool ki_json_object_set_bool(struct ki_json_object* object, const char* name, bool boolean);

// Returns true on success, false on fail.
bool ki_json_object_remove(struct ki_json_object* object, const char* name);

#pragma endregion

#pragma region Json array functions

bool ki_json_array_init(struct ki_json_array* array, size_t capacity);
void ki_json_array_fini(struct ki_json_array* array);

// Returns val at given index in json array.
// Returns NULL on fail.
struct ki_json_val* ki_json_array_at(struct ki_json_array* array, size_t index);
// Returns json object at given index in json array.
// Returns NULL on fail.
struct ki_json_object* ki_json_array_object_at(struct ki_json_array* array, size_t index);
// Returns json array at given index in json array.
// Returns NULL on fail.
struct ki_json_array* ki_json_array_array_at(struct ki_json_array* array, size_t index);
// Returns string at given index in json array.
// Returns NULL on fail.
char* ki_json_array_string_at(struct ki_json_array* array, size_t index);
// TODO: what to do on fail? ki_json_array_get_number
// Returns number at given index in json array.
// NOTE: Returns 0.0 on fail.
double ki_json_array_number_at(struct ki_json_array* array, size_t index);
// TODO: what to do on fail? ki_json_array_get_bool
// Returns bool at given index in json array.
// NOTE: Returns false on fail.
bool ki_json_array_bool_at(struct ki_json_array* array, size_t index);

// Adds json value to json array at given index.
// NOTE: Ownership of value is given to json array, and will free it once done.
// Returns true on success, false on fail.
bool ki_json_array_insert(struct ki_json_array* array, struct ki_json_val* value, size_t index);
// Creates new json value for a json object and adds it to the json array at given index.
// Returns NULL on fail.
struct ki_json_val* ki_json_array_insert_new_object(struct ki_json_array* array, size_t index, size_t capacity);
// Creates new json value for a json array and adds it to the json array at given index.
// Returns NULL on fail.
struct ki_json_val* ki_json_array_insert_new_array(struct ki_json_array* array, size_t index, size_t capacity);
// Creates new json value for a string and adds it to the json array at given index.
// NOTE: String is copied.
// Returns NULL on fail.
struct ki_json_val* ki_json_array_insert_new_string(struct ki_json_array* array, size_t index, const char* string);
// Creates new json value for a number and adds it to the json array at given index.
// Returns NULL on fail.
struct ki_json_val* ki_json_array_insert_new_number(struct ki_json_array* array, size_t index, double number);
// Creates new json value for a bool and adds it to the json array at given index.
// Returns NULL on fail.
struct ki_json_val* ki_json_array_insert_new_bool(struct ki_json_array* array, size_t index, bool boolean);
// Creates new json value representing null and adds it to the json array at given index.
// Returns NULL on fail.
struct ki_json_val* ki_json_array_insert_new_null(struct ki_json_array* array, size_t index);

// Adds json value to the end of a json array.
// NOTE: Ownership of value is given to json array, and will free it once done.
// Returns true on success, false on fail.
bool ki_json_array_add(struct ki_json_array* array, struct ki_json_val* value);
// Creates new json value for a json object and adds it to the end of a json array.
// Returns NULL on fail.
struct ki_json_val* ki_json_array_add_new_object(struct ki_json_array* array, size_t capacity);
// Creates new json value for a json array and adds it to the end of a json array.
// Returns NULL on fail.
struct ki_json_val* ki_json_array_add_new_array(struct ki_json_array* array, size_t capacity);
// Creates new json value for a string and adds it to the end of a json array.
// NOTE: String is copied.
// Returns NULL on fail.
struct ki_json_val* ki_json_array_add_new_string(struct ki_json_array* array, const char* string);
// Creates new json value for a number and adds it to the end of a json array.
// Returns NULL on fail.
struct ki_json_val* ki_json_array_add_new_number(struct ki_json_array* array, double number);
// Creates new json value for a bool and adds it to the end of a json array.
// Returns NULL on fail.
struct ki_json_val* ki_json_array_add_new_bool(struct ki_json_array* array, bool boolean);
// Creates new json value representing null and adds it to the end of a json array.
// Returns NULL on fail.
struct ki_json_val* ki_json_array_add_new_null(struct ki_json_array* array);

// NOTE 1: Value must be of type KI_JSON_VAL_STRING.
// NOTE 2: String is copied.
// Returns true on success, false on fail.
bool ki_json_array_set_string(struct ki_json_array* array, size_t index, const char* string);
// NOTE: Value must be of type KI_JSON_VAL_NUMBER.
// Returns true on success, false on fail.
bool ki_json_array_set_number(struct ki_json_array* array, size_t index, double number);
// NOTE: Value must be of type KI_JSON_VAL_BOOL.
// Returns true on success, false on fail.
bool ki_json_array_set_bool(struct ki_json_array* array, size_t index, bool boolean);

// Returns true on success, false on fail.
bool ki_json_array_remove_at(struct ki_json_array* array, size_t index);
// Removes first occurence of reference to given json value.
// Returns true on success, false on fail.
bool ki_json_array_remove(struct ki_json_array* array, struct ki_json_val* value);

#pragma endregion

#pragma region Json val functions

// Creates a json value for a json object with given starting capacity.
// Returns NULL on fail.
struct ki_json_val* ki_json_val_create_object(size_t capacity);
// Creates a json value for a json array with given starting capacity.
// Returns NULL on fail.
struct ki_json_val* ki_json_val_create_array(size_t capacity);
// Creates a json value from a string.
// NOTE: String is copied.
// Returns NULL on fail.
struct ki_json_val* ki_json_val_create_from_string(const char* string);
// Creates a json value from a double.
// Returns NULL on fail.
struct ki_json_val* ki_json_val_create_from_number(double number);
// Creates a json value from a bool.
// Returns NULL on fail.
struct ki_json_val* ki_json_val_create_from_bool(bool boolean);
// Creates a json value representing null.
// Returns NULL on fail.
struct ki_json_val* ki_json_val_create_null(void);

// NOTE 1: ki_json_value must be of type KI_JSON_VAL_STRING.
// NOTE 2: String is copied.
// Returns true on success, false on fail.
bool ki_json_val_set_string(struct ki_json_val* val, const char* string);

void ki_json_val_free(struct ki_json_val* val);

#pragma endregion
