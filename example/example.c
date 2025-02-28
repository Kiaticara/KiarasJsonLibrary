#include <stdio.h>

#include "json_node.h"

int main()
{
    printf("creating null node\n");

    struct ki_json_node* node = ki_json_node_create_null();
    
    printf("destroying null node...\n");

    ki_json_node_destroy(node);

    printf("creating number node...\n");

    node = ki_json_node_create_from_number(5.2);
    
    double* num = ki_json_node_try_get_number(node);

    if (num == NULL)
        printf("num is NULL!");
    else
        printf("number node val: %f\n", (float)*num);

    printf("destroying number node...\n");

    ki_json_node_destroy(node);

    return 0;
}