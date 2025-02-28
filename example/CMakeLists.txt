if (NOT TARGET KiarasJsonParser)
    message(FATAL_ERROR "Target is not KiarasJsonParser!")
endif()

project(KiarasJsonParserExample C)

# Generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS true)

add_executable(KiarasJsonParserExample "example.c")

target_link_libraries(KiarasJsonParserExample KiarasJsonParser)