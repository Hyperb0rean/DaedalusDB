cmake_minimum_required(VERSION 3.1...3.27)


project(
  database
  VERSION 1.0
  LANGUAGES CXX)

set(CMAKE_C_FLAGS "-g   -Werror -Wall -Wextra -pedantic -Wmissing-prototypes -Wstrict-prototypes ")
set(CMAKE_CXX_FLAGS "-g -Werror -Wall -Wextra -pedantic -Wno-error=unused-variable -Wno-error=unused-parameter -Wno-error=unused-private-field" )


set(CMAKE_CXX_STANDARD 23)

set(CMAKE_CXX_FLAGS_ASAN "-g -fsanitize=address -fsanitize=pointer-compare -fsanitize=pointer-subtract -fsanitize=leak -fsanitize=undefined -fsanitize-address-use-after-scope"
        CACHE STRING "Compiler flags in asan build"
        FORCE)

set(CMAKE_CXX_FLAGS_ASAN "-g -fsanitize=address -fsanitize=pointer-compare -fsanitize=pointer-subtract -fsanitize=leak -fsanitize=undefined -fsanitize-address-use-after-scope"
                   CACHE STRING "Compiler flags in asan build"
                   FORCE)
                   
add_executable(database main.cpp util/error.h 
util/utils.h util/utils.cpp mem/file.h mem/mem.cpp mem/mem.h db_struct/database.h db_struct/database.cpp)
