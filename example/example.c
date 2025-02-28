#include "json_node.h"

int main()
{
    //currently fails
    
    struct ki_json_node* node = ki_json_node_create_null();

    ki_json_node_destroy(node);

    return 0;
}