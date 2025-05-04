cmake_minimum_required(VERSION 3.22 FATAL_ERROR)

project(
  AirBeam
  LANGUAGES CXX
  VERSION 1.0
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/CPM.cmake)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/source/AirBeamASP")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/source/AirBeam")

# For generate compile_commands.json
add_custom_target(
  link-compile-commands ALL
  ${CMAKE_COMMAND} -E create_symlink
  ${CMAKE_BINARY_DIR}/compile_commands.json
  ${CMAKE_CURRENT_LIST_DIR}/compile_commands.json
)
