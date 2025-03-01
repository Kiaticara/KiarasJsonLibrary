#pragma once

//Functions for parsing a json string or file to a json object

//TODO: implement json parser functions

#include <stdio.h>

#include "json_object.h"

// Parse null-terminated string to json object.
// Returns NULL on fail.
struct ki_json_object* ki_json_object_parse_string(char* string);

// Parse n characters of string to json object.
// Returns NULL on fail.
struct ki_json_object* ki_json_object_nparse_string(char* string, int n);

//Returns NULL on fail.
struct ki_json_object* ki_json_object_parse_file(FILE* file);