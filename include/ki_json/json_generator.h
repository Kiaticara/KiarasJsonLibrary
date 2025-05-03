#pragma once

// Functions for generating json strings from json trees

#include "ki_json/json.h"

// Generate string from json val.
// Returned string must be freed once done.
// Returns NULL on fail.
char* ki_json_gen_string(struct ki_json_val* val);
