cmake_minimum_required(VERSION 3.3)
project(command)

file(GLOB SOURCES *.cpp)
file(GLOB HEADERS *.h)

add_library(lib_cmd ${SOURCES} ${HEADERS})
target_include_directories(lib_cmd PUBLIC "${CURRENT_CMAKE_SOURCE_DIR}/cmd.h")
