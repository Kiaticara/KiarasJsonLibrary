#pragma once

#include <stdbool.h>

#include "json_val_type.h"

struct ki_json_object;
struct ki_json_array;

//represents a node within a json tree
struct ki_json_node
{
    enum KI_JSON_VAL_TYPE type;
    
    union
    {
        struct ki_json_object* object;
        struct ki_json_array* array;
        char* string;
        double number;
        bool boolean;
    };
};

// Construction

//Returns NULL on fail.
struct ki_json_node* ki_json_node_create_from_object(struct ki_json_object* object);

//Returns NULL on fail.
struct ki_json_node* ki_json_node_create_from_array(struct ki_json_array* array);

//Returns NULL on fail.
struct ki_json_node* ki_json_node_create_from_string(char* string);

//Returns NULL on fail.
struct ki_json_node* ki_json_node_create_from_number(double number);

//Returns NULL on fail.
struct ki_json_node* ki_json_node_create_from_bool(bool boolean);

//Creates a node with KI_JSON_NODE_NULL as type. Returns NULL on fail.
struct ki_json_node* ki_json_node_create_null();

// Get data

//Returns NULL on fail.
struct ki_json_object* ki_json_node_try_get_json_object(struct ki_json_node* node);

//Returns NULL on fail.
struct ki_json_array* ki_json_node_try_get_json_array(struct ki_json_node* node);

//Returns NULL on fail.
char* ki_json_node_try_get_string(struct ki_json_node* node);

double ki_json_node_get_number(struct ki_json_node* node);

bool ki_json_node_get_bool(struct ki_json_node* node);

bool ki_json_node_is_null(struct ki_json_node* node);

// Destruction

//Frees node.
void ki_json_node_free(struct ki_json_node* node);