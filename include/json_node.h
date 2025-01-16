#pragma once

#include <stdbool.h>

//TODO: Is it okay for double, int and bool to be pointers? Seems kind of awkward...

struct json_object;
struct json_array;

enum JSON_NODE_TYPE
{
    JSON_NODE_NULL = 0, //NULL
    JSON_NODE_OBJECT = 1, //struct json_object*
    JSON_NODE_ARRAY = 2, //struct json_array*
    JSON_NODE_STRING = 3, //char*
    JSON_NODE_DOUBLE = 4, //double*
    JSON_NODE_INT = 5, //int*
    JSON_NODE_BOOL = 6 //bool*
};

//represents a node within a json tree
struct json_node
{
    enum JSON_NODE_TYPE type;
    void* data;
};

//Returns NULL on fail.
struct json_object* json_node_try_get_json_object(struct json_node* node);

//Returns NULL on fail.
struct json_array* json_node_try_get_json_array(struct json_node* node);

//Returns NULL on fail.
char* json_node_try_get_string(struct json_node* node);

//Returns NULL on fail.
double* json_node_try_get_double(struct json_node* node);

//Returns NULL on fail.
int* json_node_try_get_int(struct json_node* node);

//Returns NULL on fail.
bool* json_node_try_get_bool(struct json_node* node);
