cmake_minimum_required(VERSION 3.10)

project(KiarasJsonParser C)

# Generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS true)

set(
    LIB_SOURCES
    "src/json_node.c"
)

set(LIB_INCLUDE include)

add_library(KiarasJsonParser STATIC ${LIB_SOURCES} ${LIB_INCLUDE})

target_include_directories(KiarasJsonParser PUBLIC include)