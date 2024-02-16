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
  src/util/error.hpp 
  src/util/utils.hpp
  src/util/logger.hpp
  src/mem/file.hpp
  src/mem/page.hpp 
  src/mem/mem.hpp 
  src/mem/allocator.hpp 
  src/db_struct/database.hpp 
  src/db_struct/class_storage.hpp 
  src/db_struct/node_storage.hpp 
  src/db_struct/const_node_storage.hpp 
  src/db_struct/var_node_storage.hpp 
  src/db_struct/node.hpp 
  src/type_system/class.hpp
  src/type_system/object.hpp
  src/mem/pagelist.hpp)
add_executable(database main.cpp ${SOURCES})
target_include_directories(database PRIVATE "./src/util")
target_include_directories(database PRIVATE "./src/mem")
target_include_directories(database PRIVATE "./src/type_system")
target_include_directories(database PRIVATE "./src/db_struct")




