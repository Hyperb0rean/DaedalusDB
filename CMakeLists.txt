cmake_minimum_required(VERSION 3.1...3.27)


project(
  database
  VERSION 1.0
  LANGUAGES CXX)



  set(CMAKE_CXX_STANDARD 23)
  set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    
  set(CXX_COMMON_FLAGS "-std=c++23 -Isrc/util:/src/mem -Wall -Wextra -Werror")
  set(CXX_COMMON_FLAGS "${CXX_COMMON_FLAGS} -Weffc++ -Wsign-compare -Wshadow")
  set(CXX_COMMON_FLAGS "${CXX_COMMON_FLAGS} -Wwrite-strings -Wpointer-arith")
  # set(CXX_COMMON_FLAGS "${CXX_COMMON_FLAGS} -Winit-self -Wconversion")
  # set(CXX_COMMON_FLAGS "${CXX_COMMON_FLAGS} -Wno-sign-conversion -Wundefined-inline")
  set(CMAKE_CXX_FLAGS ${CXX_COMMON_FLAGS})
  
  set(PROJECT_ARTIFACT_APP ${PROJECT_NAME})
  set(PROJECT_ARTIFACT_TEST "${PROJECT_NAME}-test")
  

set(CMAKE_CXX_FLAGS_ASAN "${CXX_COMMON_FLAGS} -g -fsanitize=address -fsanitize=pointer-compare -fsanitize=pointer-subtract -fsanitize=leak -fsanitize=undefined -fsanitize-address-use-after-scope"
                   CACHE STRING "Compiler flags in asan build"
                   FORCE)

         
add_subdirectory(tests)
add_subdirectory(googletest)

include_directories(src)

file(GLOB_RECURSE SRC CONFIGURE_DEPENDS src/*.hpp src/*.cpp)

add_executable(${PROJECT_ARTIFACT_APP} src/main.cpp ${SRC})

set_target_properties(database PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(${PROJECT_ARTIFACT_APP} PRIVATE "src/util")
target_include_directories(${PROJECT_ARTIFACT_APP} PRIVATE "src/mem")
target_include_directories(${PROJECT_ARTIFACT_APP} PRIVATE "src/type_system")
target_include_directories(${PROJECT_ARTIFACT_APP} PRIVATE "src/db_struct")
