#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "json_node.h"

// Construction

//Returns NULL on fail.
static struct ki_json_node* ki_json_node_create(enum KI_JSON_NODE_TYPE type, void* data)
{
    struct ki_json_node* node = calloc(1, sizeof(*node));

    if (node == NULL)
    {
        printf("ki_json_node_create: allocation fail");
        return NULL;
    }

    node->type = type;
    node->data = data;

    return node;
}

//Returns NULL on fail.
struct ki_json_node* ki_json_node_create_from_object(struct ki_json_object* object)
{
    assert(object);
    
    return ki_json_node_create(KI_JSON_NODE_OBJECT, object);
}

//Returns NULL on fail.
struct ki_json_node* ki_json_node_create_from_array(struct ki_json_array* array)
{
    assert(array);
    
    return ki_json_node_create(KI_JSON_NODE_ARRAY, array);
}

//Returns NULL on fail.
struct ki_json_node* ki_json_node_create_from_string(char* string)
{
    assert(string);
    
    return ki_json_node_create(KI_JSON_NODE_STRING, string);
}

//Returns NULL on fail.
struct ki_json_node* ki_json_node_create_from_number(double number)
{
    //copy val
    double* copy = calloc(1, sizeof(*copy));
    
    if (copy == NULL)
    {
        printf("ki_json_node_create_from_number: copy allocation fail");
        return NULL;
    }

    *copy = number;

    struct ki_json_node* node = ki_json_node_create(KI_JSON_NODE_NUMBER, copy);

    //free copied val on fail
    if (node == NULL)
        free(copy);

    return node;
}

//Returns NULL on fail.
struct ki_json_node* ki_json_node_create_from_bool(bool boolean)
{
    //copy val
    bool* copy = calloc(1, sizeof(*copy));
    
    if (copy == NULL)
    {
        printf("ki_json_node_create_from_bool: copy allocation fail");
        return NULL;
    }

    *copy = boolean;

    struct ki_json_node* node = ki_json_node_create(KI_JSON_NODE_BOOL, copy);

    //free copied val on fail
    if (node == NULL)
        free(copy);

    return node;
}

//Creates a node with data == NULL and KI_JSON_NODE_NULL as type. Returns NULL on fail.
struct ki_json_node* ki_json_node_create_null()
{
    return ki_json_node_create(KI_JSON_NODE_NULL, NULL);
}

// Get data

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
double* ki_json_node_try_get_number(struct ki_json_node* node)
{
    assert(node);

    return node->data;
}

//Returns NULL on fail.
bool* ki_json_node_try_get_bool(struct ki_json_node* node)
{
    assert(node);

    return node->data;
}

bool ki_json_node_is_null(struct ki_json_node* node)
{
    assert(node);

    return node->type == KI_JSON_NODE_NULL || node->data == NULL;
}

// Destruction

void ki_json_node_destroy(struct ki_json_node* node)
{
    switch(node->type)
    {
        case KI_JSON_NODE_NUMBER:
        case KI_JSON_NODE_BOOL:

            //free copied data
            if (node->data != NULL)
                free(node->data);

            break;
        default:
            break;
    }

    free(node);
}