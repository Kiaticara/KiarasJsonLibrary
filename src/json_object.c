#include "ki_json/json_object.h"

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "ki_json/json_val_type.h"
#include "ki_json/json_node.h"

struct ki_json_object* ki_json_object_create(int capacity)
{
    struct ki_json_object* json_object = calloc(1, sizeof(*json_object));

    if (json_object == NULL)
        return NULL;

    json_object->capacity = capacity;
    json_object->num_pairs = 0;

    json_object->names = calloc(json_object->capacity, sizeof(*json_object->names));

    if (json_object->names == NULL)
    {
        free(json_object);
        return NULL;
    }

    json_object->values = calloc(json_object->capacity, sizeof(*json_object->values));

    if (json_object->values == NULL)
    {
        free(json_object->names);
        free(json_object);
        return NULL;
    }
    
    return json_object;
}

// At

//Returns node with given name inside json object. Returns NULL on fail.
struct ki_json_node* ki_json_object_node_at(struct ki_json_object* json_object, const char* name)
{
    for (int i = 0; i < json_object->num_pairs; i++)
    {
        if (strcmp(json_object->names[i], name) == 0)
            return json_object->values[i];
    }

    return NULL;
}

//Returns object with given name inside json object. Returns NULL on fail.
struct ki_json_object* ki_json_object_object_at(struct ki_json_object* json_object, const char* name)
{
    struct ki_json_node* node = ki_json_object_node_at(json_object, name);

    if (node == NULL || node->type == KI_JSON_VAL_OBJECT)
        return NULL;

    return node->object;
}

//Returns array with given name inside json object. Returns NULL on fail.
struct ki_json_array* ki_json_object_array_at(struct ki_json_object* json_object, const char* name)
{
    struct ki_json_node* node = ki_json_object_node_at(json_object, name);

    if (node == NULL || node->type == KI_JSON_VAL_ARRAY)
        return NULL;

    return node->array;
}

//Returns char* with given name inside json object. Returns NULL on fail.
char* ki_json_object_string_at(struct ki_json_object* json_object, const char* name)
{
    struct ki_json_node* node = ki_json_object_node_at(json_object, name);

    if (node == NULL || node->type == KI_JSON_VAL_STRING)
        return NULL;

    return node->string;
}

//Returns double with given name inside json object.
double ki_json_object_number_at(struct ki_json_object* json_object, const char* name)
{
    struct ki_json_node* node = ki_json_object_node_at(json_object, name);

    if (node == NULL || node->type == KI_JSON_VAL_NUMBER)
        abort();

    return node->number;
}

//Returns bool with given name inside json object.
bool ki_json_object_bool_at(struct ki_json_object* json_object, const char* name)
{
    struct ki_json_node* node = ki_json_object_node_at(json_object, name);

    if (node == NULL || node->type == KI_JSON_VAL_BOOL)
        abort();

    return node->boolean;
}

// Is

bool ki_json_object_at_is_type(struct ki_json_object* json_object, const char* name, enum KI_JSON_VAL_TYPE type)
{
    struct ki_json_node* node = ki_json_object_node_at(json_object, name);

    return (node != NULL && node->type == type);
}

bool ki_json_object_at_is_type_or_null(struct ki_json_object* json_object, const char* name, enum KI_JSON_VAL_TYPE type)
{
    struct ki_json_node* node = ki_json_object_node_at(json_object, name);

    return (node != NULL && (node->type == type || node->type == KI_JSON_VAL_NULL));
}

// Adding

static bool ki_json_object_expand(struct ki_json_object* json_object)
{
    // allocate new arrays

    char** new_names = realloc(json_object->names, sizeof(*new_names) * json_object->capacity * 2);

    if (json_object->names == NULL)
        return false;

    struct ki_json_node** new_values = realloc(json_object->values, sizeof(*new_values) * json_object->capacity * 2);

    if (new_values == NULL)
    {
        free(new_names);
        return false;
    }

    // fill new spots with NULL
    for (int i = json_object->capacity; i < json_object->capacity * 2; i++)
    {
        new_names[i] = NULL;
        new_values[i] = NULL;
    }

    // set to new

    json_object->names = new_names;
    json_object->values = new_values;
    
    json_object->capacity *= 2;

    return true;
}

//Returns true on success
bool ki_json_object_add_node(struct ki_json_object* json_object, char* name, struct ki_json_node* node)
{
    //if need to expand, but failed to do so
    if (json_object->num_pairs == json_object->capacity && !ki_json_object_expand(json_object))
        return false;
    
    json_object->names[json_object->num_pairs] = name;
    json_object->values[json_object->num_pairs] = node; 
    json_object->num_pairs++;

    return true;
}

//Returns true on success
bool ki_json_object_add_object(struct ki_json_object* json_object, char* name, struct ki_json_object* object)
{
    struct ki_json_node* node = ki_json_node_create_from_object(object);

    if (node == NULL)
        return false;

    if (!ki_json_object_add_node(json_object, name, node))
    {
        ki_json_node_free(node);
        return false;
    }

    return true;
}

//Returns true on success
bool ki_json_object_add_array(struct ki_json_object* json_object, char* name, struct ki_json_array* array)
{
    struct ki_json_node* node = ki_json_node_create_from_array(array);

    if (node == NULL)
        return false;

    if (!ki_json_object_add_node(json_object, name, node))
    {
        ki_json_node_free(node);
        return false;
    }

    return true;
}

//Returns true on success
bool ki_json_object_add_string(struct ki_json_object* json_object, char* name, char* string)
{
    struct ki_json_node* node = ki_json_node_create_from_string(string);

    if (node == NULL)
        return false;

    if (!ki_json_object_add_node(json_object, name, node))
    {
        ki_json_node_free(node);
        return false;
    }

    return true;
}

//Returns true on success
bool ki_json_object_add_number(struct ki_json_object* json_object, char* name, double number)
{
    struct ki_json_node* node = ki_json_node_create_from_number(number);

    if (node == NULL)
        return false;

    if (!ki_json_object_add_node(json_object, name, node))
    {
        ki_json_node_free(node);
        return false;
    }

    return true;
}

//Returns true on success
bool ki_json_object_add_bool(struct ki_json_object* json_object, char* name, bool boolean)
{
    struct ki_json_node* node = ki_json_node_create_from_bool(boolean);

    if (node == NULL)
        return false;

    if (!ki_json_object_add_node(json_object, name, node))
    {
        ki_json_node_free(node);
        return false;
    }

    return true;
}

//Returns true on success
bool ki_json_object_add_null(struct ki_json_object* json_object, char* name)
{
    struct ki_json_node* node = ki_json_node_create_null();

    if (node == NULL)
        return false;

    if (!ki_json_object_add_node(json_object, name, node))
    {
        ki_json_node_free(node);
        return false;
    }

    return true;
}

// Destruction

//Free json object & its nodes
void ki_json_object_free(struct ki_json_object* json_object)
{
    for (int i = 0; i < json_object->num_pairs; i++)
        ki_json_node_free(json_object->values[i]);

    free(json_object->names);
    free(json_object->values);

    free(json_object);
}