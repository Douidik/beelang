cmake_minimum_required(VERSION 3.20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(
  bee
  DESCRIPTION "Bee programming language compiler"
  LANGUAGES CXX
)

project(
  bee-cmd
  DESCRIPTION "Bee compiler command line interface"
  LANGUAGES CXX
)

project(
  qcc-test
  DESCRIPTION "Bee test suite"
  LANGUAGES CXX
)

add_subdirectory(bee)
add_subdirectory(cmd)
add_subdirectory(test)

