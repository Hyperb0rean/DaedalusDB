cmake_minimum_required(VERSION 3.1...3.27)


project(
  lab1
  VERSION 1.0
  LANGUAGES C)

set(CMAKE_C_FLAGS "-Wall -Wpedantic")

add_executable(lab1 main.c)
