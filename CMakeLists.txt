cmake_minimum_required(VERSION 3.10)

project(KiarasJsonLibrary C)

# Generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS true)

set(
    LIB_SOURCES
    "src/json_node.c"
    "src/json_object.c"
)

set(LIB_INCLUDE include)

add_library(KiarasJsonLibrary STATIC ${LIB_SOURCES} ${LIB_INCLUDE})

target_include_directories(KiarasJsonLibrary PUBLIC ${LIB_INCLUDE})

add_subdirectory(example)