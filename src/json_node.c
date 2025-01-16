#include <assert.h>

#include "json_node.h"

#ifndef NULL
#define NULL ((void*)0)
#endif

//Returns NULL on fail.
struct ki_json_object* ki_json_node_try_get_json_object(struct ki_json_node* node)
{
    assert(node);

    if (node->type != KI_JSON_NODE_OBJECT)
        return NULL;

    return node->data;
}

//Returns NULL on fail.
struct ki_json_array* ki_json_node_try_get_json_array(struct ki_json_node* node)
{
    assert(node);

    if (node->type != KI_JSON_NODE_ARRAY)
        return NULL;

    return node->data;
}

//Returns NULL on fail.
char* ki_json_node_try_get_string(struct ki_json_node* node)
{
    assert(node);

    if (node->type != KI_JSON_NODE_STRING)
        return NULL;

    return node->data;
}

//Returns NULL on fail.
double* ki_json_node_try_get_double(struct ki_json_node* node)
{
    assert(node);

    if (node->type != KI_JSON_NODE_DOUBLE)
        return NULL;

    return node->data;
}

//Returns NULL on fail.
bool* ki_json_node_try_get_bool(struct ki_json_node* node)
{
    assert(node);

    if (node->type != KI_JSON_NODE_BOOL)
        return NULL;

    return node->data;
}
