cmake_minimum_required(VERSION 3.3)
project(command)

add_subdirectory(lib_cmd)
add_subdirectory(unit_tests)

add_executable(test_cmd main.cpp)
target_link_libraries(test_cmd lib_cmd)
