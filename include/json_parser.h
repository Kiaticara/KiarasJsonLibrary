#pragma once

//Functions for parsing a json string or file to a json object

//TODO: implement json parser functions

#include <stdio.h>

#include "json_object.h"

//Returns NULL on fail.
struct ki_json_object* ki_json_object_parse_string(const char* string);

//Returns NULL on fail.
struct ki_json_object* ki_json_object_parse_file(FILE* file);