#include "ki_json/json.h"

#include <stdlib.h>
#include <assert.h>

bool ki_json_array_init(struct ki_json_array* array, size_t capacity)
{
    assert(array);

    array->capacity = capacity;
    array->count = 0;

    array->values = calloc(array->capacity, sizeof(*array->values));

    if (array->values == NULL)
    {
        free(array);
        return false;
    }
    
    return true;
}

void ki_json_array_fini(struct ki_json_array* array)
{
    assert(array);

    if (array->values != NULL)
    {
        for (int i = 0; i < array->count; i++)
            ki_json_val_free(array->values[i]);

        free(array->values);
        array->values = NULL;
    }

    array->count = 0;
    array->capacity = 0;  
}