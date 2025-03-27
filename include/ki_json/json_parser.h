#pragma once

//Functions for parsing a json string or file to a json tree

//TODO: implement json parser functions

#include <stdio.h>

#include "ki_json/json_node.h"

// Parse null-terminated string to a json tree.
// Returns NULL on fail.
struct ki_json_node* ki_json_tree_parse_string(char* string);

// Parse n characters of string to a json tree.
// Returns NULL on fail.
struct ki_json_node* ki_json_tree_nparse_string(char* string, int n);

//Returns NULL on fail.
struct ki_json_node* ki_json_tree_parse_file(FILE* file);