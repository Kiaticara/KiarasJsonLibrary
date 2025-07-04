#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "ki_json/json.h"
#include "ki_json/json_parser.h"
#include "ki_json/json_generator.h"

static void print_val(struct ki_json_val* val)
{
    assert(val);

    char* string = ki_json_gen_string(val);

    if (string != NULL)
    {
        printf("%s\n", string);
        free(string);
    }
    else
    {
        printf("failed to gen string...\n");
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

        struct ki_json_parser_err err = {0};
        struct ki_json_val* val = ki_json_nparse_string(buffer, amount, &err);
        free(buffer);

        if (val != NULL)
        {
            printf("parsed value!\n");

            printf("printing value using json generator...\n");

            print_val(val);

            printf("freeing value %s...\n", path);
            ki_json_val_free(val);
            printf("freed!\n");
        }
        else 
        {
            printf("err: %i\n", err.type);
            printf("failed to parse value...\n");
        }
    }
}
