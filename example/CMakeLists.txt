if (NOT TARGET KiarasJsonLibrary)
    message(FATAL_ERROR "No target KiarasJsonLibrary!")
endif()

project(KiarasJsonLibraryExample C)

set(CMAKE_BUILD_TYPE DEBUG)

# Generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS true)

add_executable(KiarasJsonLibraryExample "example.c")

set_target_properties(KiarasJsonLibraryExample PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
target_compile_options(KiarasJsonLibraryExample PRIVATE -Wall -Wextra -Wpedantic) #-fsanitize=address

target_link_libraries(KiarasJsonLibraryExample KiarasJsonLibrary)

#example 2

add_executable(KiarasJsonLibraryExample2 "example2.c")

set_target_properties(KiarasJsonLibraryExample2 PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
target_compile_options(KiarasJsonLibraryExample2 PRIVATE -Wall -Wextra -Wpedantic) #-fsanitize=address

target_link_libraries(KiarasJsonLibraryExample2 KiarasJsonLibrary)