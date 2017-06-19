cmake_minimum_required(VERSION 3.3)
project(command)

file(GLOB SOURCES *.cpp)
file(GLOB HEADERS *.h)

add_library(lib_cmd ${SOURCES} ${HEADERS})
