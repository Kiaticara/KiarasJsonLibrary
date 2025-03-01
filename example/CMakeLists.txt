if (NOT TARGET KiarasJsonLibrary)
    message(FATAL_ERROR "No target KiarasJsonLibrary!")
endif()

project(KiarasJsonLibraryExample C)

# Generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS true)

add_executable(KiarasJsonLibraryExample "example.c")

target_link_libraries(KiarasJsonLibraryExample KiarasJsonLibrary)