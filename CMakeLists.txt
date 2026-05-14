cmake_minimum_required(VERSION 3.16)
project(LexorProject CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Collect all sources
add_executable(lexor
    src/main.cpp
    src/Lexer/Lexer.cpp
    src/Parser/Parser.cpp
    src/Runtime/Interpreter.cpp
)

# Make all src/ subdirectories available as include paths
target_include_directories(lexor PRIVATE src)

# Enable warnings
if (MSVC)
    target_compile_options(lexor PRIVATE /W4)
else()
    target_compile_options(lexor PRIVATE -Wall -Wextra -Wpedantic)
endif()
