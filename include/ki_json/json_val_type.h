#pragma once

enum KI_JSON_VAL_TYPE
{
    KI_JSON_VAL_NULL = 0, //nothing, NULL
    KI_JSON_VAL_OBJECT = 1, //object, struct ki_json_object*
    KI_JSON_VAL_ARRAY = 2, //array, struct ki_json_array*
    KI_JSON_VAL_STRING = 3, //string, char*
    KI_JSON_VAL_NUMBER = 4, //number, double
    KI_JSON_VAL_BOOL = 5 //boolean, bool
};