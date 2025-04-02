# Kiara's Json Library (ki_json)

A library for parsing & generating json strings.

Notes:

- Supports utf8 only
- \0 (NULL character) is not supported

## Headers

| Header file | Description |
| --- | --- |
| json.h | functions for and representation of json values & trees, including json objects & json arrays |
| json_parser.h | functions for parsing json strings to ki_json's representation of them |
| json_generator.h | functions for generating json strings from ki_json's representation of them |

## Building (using cmake & UNIX Makefiles)

    cmake -S . -B build
    cd build
    make

## Refs

- https://www.json.org/json-en.html
- https://www.rfc-editor.org/rfc/rfc4627
- https://json.org/example.html