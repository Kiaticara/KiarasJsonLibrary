#pragma once

#include <stdbool.h>

//TODO: Is it okay for double and bool to be pointers? Seems kind of awkward...

struct ki_json_object;
struct ki_json_array;

enum KI_JSON_NODE_TYPE
{
    KI_JSON_NODE_NULL = 0, //NULL
    KI_JSON_NODE_OBJECT = 1, //struct ki_json_object*
    KI_JSON_NODE_ARRAY = 2, //struct ki_json_array*
    KI_JSON_NODE_STRING = 3, //char*
    KI_JSON_NODE_DOUBLE = 4, //double*
    KI_JSON_NODE_BOOL = 5 //bool*
};

//represents a node within a json tree
struct ki_json_node
{
    enum KI_JSON_NODE_TYPE type;
    void* data;
};

//TODO: construction & destruction

//Returns NULL on fail.
struct ki_json_object* ki_json_node_try_get_json_object(struct ki_json_node* node);

//Returns NULL on fail.
struct ki_json_array* ki_json_node_try_get_json_array(struct ki_json_node* node);

//Returns NULL on fail.
char* ki_json_node_try_get_string(struct ki_json_node* node);

//Returns NULL on fail.
double* ki_json_node_try_get_double(struct ki_json_node* node);

//Returns NULL on fail.
bool* ki_json_node_try_get_bool(struct ki_json_node* node);