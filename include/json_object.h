#pragma once

#include "json_node.h"

//A collection of name/value pairs.
struct ki_json_object
{
    char** names;
    struct ki_json_node** values;
    int num_pairs;
};

//TODO: implement ki_json_object

//Returns node with given name inside json object. Returns NULL on fail.
struct ki_json_node* ki_json_object_get_node(const char* name);

//Free json object & its nodes
void ki_json_object_destroy(struct ki_json_object* json_object);