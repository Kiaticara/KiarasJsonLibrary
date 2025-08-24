#ifndef KI_JSON_GENERATOR_H
#define KI_JSON_GENERATOR_H

// Functions for generating json strings from json trees

#include "ki_json/json.h"

#ifdef __cplusplus
extern "C"
{
#endif

// Generate string from json val.
// Returned string must be freed once done.
// Returns NULL on fail.
char* ki_json_gen_string(struct ki_json_val* val);

#ifdef __cplusplus
}
#endif

#endif //KI_JSON_GENERATOR_H
