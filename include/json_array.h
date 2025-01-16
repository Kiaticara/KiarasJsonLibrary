#pragma once

#include "json_node.h"

//An ordered list of values.
struct ki_json_array
{
    struct ki_json_node** values;
    int length;
};

//TODO: implement ki_json_array