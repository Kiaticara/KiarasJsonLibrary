#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "ki_json/json.h"
#include "ki_json/json_parser.h"

static void print_val(struct ki_json_val* val, int depth)
{
    assert(val);

    switch(val->type)
    {
        case KI_JSON_VAL_OBJECT:
            printf("{\n");
            for (size_t i = 0; i < val->value.object.count; i++)
            {  
                for (int j = 0; j <= depth; j++)
                    printf("\t");

                printf("%s: ", val->value.object.names[i]);
                print_val(val->value.object.values[i], depth + 1);
                printf("\n");
            }

            for (int i = 0; i < depth; i++)
                    printf("\t");

            printf("}");
            break;
        case KI_JSON_VAL_ARRAY:
            printf("[\n");
            for (size_t i = 0; i < val->value.array.count; i++)
            {  
                for (int j = 0; j <= depth; j++)
                    printf("\t");

                printf("%zu: ", i);
                print_val(val->value.array.values[i], depth + 1);
                printf("\n");
            }

            for (int i = 0; i < depth; i++)
                    printf("\t");

            printf("]");
            break;
        case KI_JSON_VAL_STRING:
            printf("\"%s\"", val->value.string);
            break;
        case KI_JSON_VAL_NUMBER:
            printf("%f", val->value.number);
            break;
        case KI_JSON_VAL_BOOL:
            printf("%i", val->value.boolean);
            break;
        case KI_JSON_VAL_NULL:
            printf("null");
            break;
    }
}

static long file_len(FILE* file)
{
    assert(file);

    //position to return to after getting length of file
    long pos = ftell(file);

    //go to the end of file and get position -> length
    fseek(file, 0, SEEK_END);
    long length = ftell(file);

    //go back
    fseek(file, pos, SEEK_SET);

    return length;
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        printf("No files to parse & print supplied!\n");
        return 1;
    }

    for (int i = 1; i < argc; i++)
    {
        char* path = argv[i];
    
        printf("reading %s\n", argv[1]);

        FILE* file = fopen(path, "r");

        if (file == NULL)
        {
            printf("file %s couldn't be opened!\n", path);
            continue;
        }

        size_t file_size = (size_t)file_len(file);

        printf("file size: %zu\n", file_size);

        char* buffer = calloc(file_size, sizeof(*buffer));

        if (buffer == NULL)
        {
            fclose(file);
            printf("couldn't allocate %zu bytes for buffer\n", file_size);
            continue;
        }

        size_t amount = fread(buffer, sizeof(*buffer), file_size, file);
        fclose(file);

        printf("parsing %s\n", path);

        struct ki_json_val* val = ki_json_nparse_string(buffer, amount);
        free(buffer);

        if (val != NULL)
        {
            printf("parsed value!\n");

            print_val(val, 0);
            printf("\n");

            printf("freeing value %s...\n", path);
            ki_json_val_free(val);
            printf("freed!\n");
        }
        else 
        {
            printf("failed to parse value...\n");
        }
    }
}
