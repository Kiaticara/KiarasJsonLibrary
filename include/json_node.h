enum JSON_NODE_TYPE
{
    JSON_NODE_NULL = 0, //NULL
    JSON_NODE_OBJECT = 1, //
    JSON_NODE_ARRAY = 2, //struct json_array*
    JSON_NODE_STRING = 3, //struct json_string*
    JSON_NODE_NUMBER = 4, 
    JSON_NODE_BOOLEAN = 5
}

//represents a node within a json tree
struct json_node
{
    enum JSON_NODE_TYPE type;
    void* data;
}