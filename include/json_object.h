#pragma once

#include "json_node.h"

//A collection of name/value pairs.
struct json_object
{
    char** names;
    struct json_node* values;
    int num_pairs;
};

//Returns node with given name inside json object. Returns NULL on fail.
struct json_node* json_object_get_node(const char* name);