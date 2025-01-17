#pragma once

#include <stdbool.h>

#include "json_array.h"

struct ki_json_node;

//A collection of name/value pairs.
struct ki_json_object
{
    //Names of pairs, may be NULL if num_pairs == 0
    char** names;
    //Values of pairs, may be NULL if num_pairs == 0
    struct ki_json_node** values;
    //Number of pairs, duh.
    int num_pairs;
};


//TODO: implement ki_json_object

// At

//Returns node with given name inside json object. Returns NULL on fail.
struct ki_json_node* ki_json_object_node_at(struct ki_json_object* json_object, const char* name);

//Returns object with given name inside json object. Returns NULL on fail.
struct ki_json_object* ki_json_object_object_at(struct ki_json_object* json_object, const char* name);

//Returns array with given name inside json object. Returns NULL on fail.
struct ki_json_array* ki_json_object_array_at(struct ki_json_object* json_object, const char* name);

//Returns char* with given name inside json object. Returns NULL on fail.
char* ki_json_object_string_at(struct ki_json_object* json_object, const char* name);

//Returns double with given name inside json object.
double ki_json_object_number_at(struct ki_json_object* json_object, const char* name);

//Returns bool with given name inside json object.
bool ki_json_object_bool_at(struct ki_json_object* json_object, const char* name);

// Is

bool ki_json_object_is_object(struct ki_json_object* json_object, const char* name);

bool ki_json_object_is_array(struct ki_json_object* json_object, const char* name);

bool ki_json_object_is_string(struct ki_json_object* json_object, const char* name);

bool ki_json_object_is_number(struct ki_json_object* json_object, const char* name);

bool ki_json_object_is_bool(struct ki_json_object* json_object, const char* name);

bool ki_json_object_is_null(struct ki_json_object* json_object, const char* name);

// Adding

//Returns 0 on success
int ki_json_object_add_node(struct ki_json_object* json_object, const char* name, struct ki_json_node* node);

//Returns 0 on success
int ki_json_object_add_object(struct ki_json_object* json_object, const char* name, struct ki_json_object* object);

//Returns 0 on success
int ki_json_object_add_array(struct ki_json_object* json_object, const char* name, struct ki_json_array* array);

//Returns 0 on success
int ki_json_object_add_string(struct ki_json_object* json_object, const char* name, const char* string);

//Returns 0 on success
int ki_json_object_add_number(struct ki_json_object* json_object, const char* name, double number);

//Returns 0 on success
int ki_json_object_add_bool(struct ki_json_object* json_object, const char* name, bool boolean);

//Returns 0 on success
int ki_json_object_add_null(struct ki_json_object* json_object, const char* name);

// Destruction

//Free json object & its nodes
void ki_json_object_destroy(struct ki_json_object* json_object);