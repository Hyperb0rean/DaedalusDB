cmake_minimum_required(VERSION 3.1...3.27)


project(
  lab1
  VERSION 1.0
  LANGUAGES C)

set(CMAKE_C_FLAGS "-Werror -Wall -Wextra -pedantic -Wmissing-prototypes -Wstrict-prototypes ")


set(CMAKE_C_FLAGS_ASAN "-g -fsanitize=address -fsanitize=pointer-compare -fsanitize=pointer-subtract -fsanitize=leak -fsanitize=undefined -fsanitize-address-use-after-scope"
                   CACHE STRING "Compiler flags in asan build"
                   FORCE)
                   
add_executable(database main.c io/io.c io/io.h)
