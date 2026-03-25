# Enhanced Unit Test CMakeLists.txt
cmake_minimum_required(VERSION 3.12)

add_subdirectory(interpreter)
if (NOT WAMR_BUILD_TARGET STREQUAL "X86_32")
  add_subdirectory(constants)
  add_subdirectory(control-flow)
  add_subdirectory(extension)
  add_subdirectory(memory)
  add_subdirectory(numeric)
endif ()
