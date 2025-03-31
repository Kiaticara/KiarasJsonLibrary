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

set_target_properties(KiarasJsonLibrary PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)

target_include_directories(KiarasJsonLibrary PUBLIC ${LIB_INCLUDE})

add_subdirectory(example)