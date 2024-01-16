cmake_minimum_required(VERSION 3.1...3.27)


project(
  database
  VERSION 1.0
  LANGUAGES CXX)

set(CMAKE_CXX_FLAGS "-g -Werror -Wall -Wextra -pedantic -Wno-error=unused-variable -Wno-error=unused-parameter " )
set(CMAKE_CXX_FLAGS_ASAN "-g -fsanitize=address -fsanitize=pointer-compare -fsanitize=pointer-subtract -fsanitize=leak -fsanitize=undefined -fsanitize-address-use-after-scope"
                   CACHE STRING "Compiler flags in asan build"
                   FORCE)
set(CMAKE_CXX_STANDARD 23)

         
enable_testing()
add_subdirectory(tests)
add_subdirectory(googletest)


set(SOURCES 
  src/util/error.h 
  src/util/utils.h
  src/mem/file.h
  src/mem/page.h 
  src/mem/mem.h 
  src/mem/allocator.h 
  src/db_struct/database.h 
  src/db_struct/types.h)
add_executable(database main.cpp ${SOURCES})
target_include_directories(database PRIVATE "./src/mem")
target_include_directories(database PRIVATE "./src/db_struct")
target_include_directories(database PRIVATE "./src/util")



