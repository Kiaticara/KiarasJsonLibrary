#pragma once

//Functions for parsing a json string to a json tree

//TODO: implement json parser functions

#include <stddef.h>
#include <stdio.h>

#include "ki_json/json.h"

// Parse null-terminated string to a json tree.
// Returns NULL on fail.
struct ki_json_val* ki_json_tree_parse_string(char* string);

// Parse n characters of string to a json tree.
// Returns NULL on fail.
struct ki_json_val* ki_json_tree_nparse_string(char* string, size_t n);