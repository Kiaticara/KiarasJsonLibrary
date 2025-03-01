#include <stdio.h>

#include "ki_json/json_node.h"
#include "ki_json/json_object.h"

int main()
{
    //TODO: example should do null checks for json objects & nodes
    
    printf("creating json object\n");

    struct ki_json_object* object = ki_json_object_create(1);

    printf("creating null node\n");

    struct ki_json_node* node_null = ki_json_node_create_null();
    
    printf("creating 2 number node...\n");

    struct ki_json_node* node_num1 = ki_json_node_create_from_number(5.2);
    struct ki_json_node* node_num2 = ki_json_node_create_from_number(-202.2);
    
    printf("number node 1 val: %f\n", (float)ki_json_node_get_number(node_num1));

    printf("number node 2 val: %f\n", (float)ki_json_node_get_number(node_num2));

    printf("adding existing nodes to json object\n");

    ki_json_object_add_node(object, "test1", node_null);
    ki_json_object_add_node(object, "test2", node_num1);
    ki_json_object_add_node(object, "test3", node_num2);

    printf("adding new nodes to json object\n");

    ki_json_object_add_bool(object, "test4", true);
    ki_json_object_add_string(object, "test5", "abc");
    ki_json_object_add_number(object, "test6", 3.3);
    
    printf("destroying json object\n");
    ki_json_object_free(object);

    return 0;
}