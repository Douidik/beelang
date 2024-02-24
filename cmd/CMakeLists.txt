file(
  GLOB_RECURSE CMD_SOURCE
  "[a-z0-9]" *.hpp
  "[a-z0-9]" *.cpp
)

add_executable(
  bee-cmd
  ${CMD_SOURCE}
)

target_include_directories(
  bee-cmd PRIVATE
  ${CMAKE_SOURCE_DIR}/bee
  ${CMAKE_SOURCE_DIR}/cmd
)

target_link_libraries(
  bee-cmd PRIVATE
  bee
)

set_target_properties(
  bee-cmd PROPERTIES
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED YES
  LINKER_LANGUAGE CXX
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
