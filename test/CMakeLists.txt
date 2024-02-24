file(
  GLOB_RECURSE TEST_SOURCE
  "[a-z0-9]" *.hpp
  "[a-z0-9]" *.cpp
)

add_executable(
  bee-test
  ${TEST_SOURCE}
)

target_include_directories(
  bee-test PRIVATE
  ${CMAKE_SOURCE_DIR}/bee
  ${CMAKE_SOURCE_DIR}/test
)

target_link_libraries(
  bee-test PRIVATE
  bee
)

set_target_properties(
  bee-test PROPERTIES
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED YES
  LINKER_LANGUAGE CXX
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
