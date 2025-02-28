cmake_minimum_required(VERSION 3.10)

project(KiarasJsonParserExample)

# Generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS true)

add_executable(KiarasJsonParserExample "example.c")

if (EXISTS "../../build/libKiarasJsonParser.a")
    message(ERROR "Library libKiarasJsonParser.a not found!")
endif()

target_link_libraries(KiarasJsonParserExample "../../build/libKiarasJsonParser.a")