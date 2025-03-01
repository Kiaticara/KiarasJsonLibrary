#pragma once

#include <stdbool.h>

#include "json_array.h"

struct ki_json_node;

// A collection of json name/value pairs.
struct ki_json_object
{
    // Names of pairs
    char** names;
    // Values of pairs
    struct ki_json_node** values;
    // Number of pairs currently in this json object
    int num_pairs;
    // Maximum amount of pairs this json object can currently hold.
    // Expands automatically, will never shrink
    int capacity;
};

// Starting capacity must be larger than 0. Returns NULL on fail.
struct ki_json_object* ki_json_object_create(int capacity);

// At

// Returns node with given name inside json object. Returns NULL on fail.
struct ki_json_node* ki_json_object_node_at(struct ki_json_object* json_object, const char* name);

// Returns object with given name inside json object. Returns NULL on fail.
struct ki_json_object* ki_json_object_object_at(struct ki_json_object* json_object, const char* name);

// Returns array with given name inside json object. Returns NULL on fail.
struct ki_json_array* ki_json_object_array_at(struct ki_json_object* json_object, const char* name);

// Returns char* with given name inside json object. Returns NULL on fail.
char* ki_json_object_string_at(struct ki_json_object* json_object, const char* name);

// Returns double with given name inside json object.
double ki_json_object_number_at(struct ki_json_object* json_object, const char* name);

// Returns bool with given name inside json object.
bool ki_json_object_bool_at(struct ki_json_object* json_object, const char* name);

// Is

bool ki_json_object_is_object(struct ki_json_object* json_object, const char* name);

bool ki_json_object_is_array(struct ki_json_object* json_object, const char* name);

bool ki_json_object_is_string(struct ki_json_object* json_object, const char* name);

bool ki_json_object_is_number(struct ki_json_object* json_object, const char* name);

bool ki_json_object_is_bool(struct ki_json_object* json_object, const char* name);

bool ki_json_object_is_null(struct ki_json_object* json_object, const char* name);

// Adding

// Object takes ownership of given node.
// Returns true on success
bool ki_json_object_add_node(struct ki_json_object* json_object, char* name, struct ki_json_node* node);

// Returns true on success
bool ki_json_object_add_object(struct ki_json_object* json_object, char* name, struct ki_json_object* object);

// Returns true on success
bool ki_json_object_add_array(struct ki_json_object* json_object, char* name, struct ki_json_array* array);

// Returns true on success
bool ki_json_object_add_string(struct ki_json_object* json_object, char* name, char* string);

// Returns true on success
bool ki_json_object_add_number(struct ki_json_object* json_object, char* name, double number);

// Returns true on success
bool ki_json_object_add_bool(struct ki_json_object* json_object, char* name, bool boolean);

// Returns true on success
bool ki_json_object_add_null(struct ki_json_object* json_object, char* name);

// Destruction

// Free json object & its nodes
void ki_json_object_free(struct ki_json_object* json_object);