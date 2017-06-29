cmake_minimum_required(VERSION 3.3)
project(command)

file(GLOB SOURCES *.cpp)
file(GLOB HEADERS *.h)

add_library(lib_cmd
    ${SOURCES} ${HEADERS})

target_include_directories(
    lib_cmd PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/cmd.h")

find_package(
    PythonInterp 2.7)

if (PYTHONINTERP_FOUND)
    set(
        DOC_GEN_PY
        ${CMAKE_CURRENT_SOURCE_DIR}/doc_gen.py
    )
    add_custom_target(
        cmd_docs
        COMMAND ${PYTHON_EXECUTABLE} ${DOC_GEN_PY}
        DEPENDS lib_cmd
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        SOURCES ${DOC_GEN_PY}
    )
endif()
