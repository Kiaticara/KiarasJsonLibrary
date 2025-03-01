#include "ki_json/json_node.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "ki_json/json_val_type.h"

// Construction

//Returns NULL on fail.
struct ki_json_node* ki_json_node_create_from_object(struct ki_json_object* object)
{
    assert(object);
    
    struct ki_json_node* node = calloc(1, sizeof(*node));

    if (node == NULL)
    {
        printf("ki_json_node_create_from_object: allocation fail");
        return NULL;
    }

    node->type = KI_JSON_VAL_OBJECT;
    node->object = object;

    return node;
}

//Returns NULL on fail.
struct ki_json_node* ki_json_node_create_from_array(struct ki_json_array* array)
{
    assert(array);
    
    struct ki_json_node* node = calloc(1, sizeof(*node));

    if (node == NULL)
    {
        printf("ki_json_node_create_from_array: allocation fail");
        return NULL;
    }

    node->type = KI_JSON_VAL_ARRAY;
    node->array = array;

    return node;
}

//Returns NULL on fail.
struct ki_json_node* ki_json_node_create_from_string(char* string)
{
    assert(string);
    
    struct ki_json_node* node = calloc(1, sizeof(*node));

    if (node == NULL)
    {
        printf("ki_json_node_create_from_string: allocation fail");
        return NULL;
    }

    node->type = KI_JSON_VAL_STRING;
    node->string = string;

    return node;
}

//Returns NULL on fail.
struct ki_json_node* ki_json_node_create_from_number(double number)
{
    struct ki_json_node* node = calloc(1, sizeof(*node));

    if (node == NULL)
    {
        printf("ki_json_node_create_from_number: allocation fail");
        return NULL;
    }

    node->type = KI_JSON_VAL_NUMBER;
    node->number = number;

    return node;
}

//Returns NULL on fail.
struct ki_json_node* ki_json_node_create_from_bool(bool boolean)
{
    struct ki_json_node* node = calloc(1, sizeof(*node));

    if (node == NULL)
    {
        printf("ki_json_node_create_from_bool: allocation fail");
        return NULL;
    }

    node->type = KI_JSON_VAL_BOOL;
    node->boolean = boolean;

    return node;
}

//Creates a node with KI_JSON_NODE_NULL as type. Returns NULL on fail.
struct ki_json_node* ki_json_node_create_null()
{
    struct ki_json_node* node = calloc(1, sizeof(*node));

    if (node == NULL)
    {
        printf("ki_json_node_create_null: allocation fail");
        return NULL;
    }

    node->type = KI_JSON_VAL_NULL;
    node->object = NULL;

    return node;
}

// Get data

//Returns NULL on fail.
struct ki_json_object* ki_json_node_try_get_json_object(struct ki_json_node* node)
{
    assert(node);

    if (node->type != KI_JSON_VAL_OBJECT)
        return NULL;

    return node->object;
}

//Returns NULL on fail.
struct ki_json_array* ki_json_node_try_get_json_array(struct ki_json_node* node)
{
    assert(node);

    if (node->type != KI_JSON_VAL_ARRAY)
        return NULL;

    return node->array;
}

//Returns NULL on fail.
char* ki_json_node_try_get_string(struct ki_json_node* node)
{
    assert(node);

    if (node->type != KI_JSON_VAL_STRING)
        return NULL;

    return node->string;
}

double ki_json_node_get_number(struct ki_json_node* node)
{
    assert(node && node->type == KI_JSON_VAL_NUMBER);

    return node->number;
}

bool ki_json_node_get_bool(struct ki_json_node* node)
{
    assert(node && node->type == KI_JSON_VAL_BOOL);

    return node->boolean;
}

bool ki_json_node_is_null(struct ki_json_node* node)
{
    assert(node);

    return node->type == KI_JSON_VAL_NULL || node->object == NULL;
}

// Destruction

void ki_json_node_free(struct ki_json_node* node)
{
    assert(node);

    free(node);
}