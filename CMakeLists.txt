cmake_minimum_required(VERSION 3.10)

project(KiarasJsonLibrary C)

# Generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS true)

set(
    LIB_SOURCES
    "src/json/json_val.c"
    "src/json/json_object.c"
    "src/json/json_array.c"
    "src/json_parser.c"
    "src/json_generator.c"
)

set(LIB_INCLUDE include)

add_library(KiarasJsonLibrary STATIC ${LIB_SOURCES} ${LIB_INCLUDE})

target_compile_options(KiarasJsonLibrary PUBLIC -Wall -Wextra -Wpedantic)

set_target_properties(KiarasJsonLibrary PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)

target_include_directories(KiarasJsonLibrary PUBLIC ${LIB_INCLUDE})

add_subdirectory(example)