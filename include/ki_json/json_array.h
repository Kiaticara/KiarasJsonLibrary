#pragma once

#include "json_node.h"

//An ordered list of values.
struct ki_json_array
{
    struct ki_json_node** values;
    // Number of values currently in this json object
    int count;
    // Maximum amount of values this json object can currently hold.
    // Expands automatically, will never shrink
    int capacity;
};

//TODO: implement ki_json_array