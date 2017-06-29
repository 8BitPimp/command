file(GLOB SOURCES *.cpp)
file(GLOB HEADERS *.h)

add_executable(unit_tests ${SOURCES} ${HEADERS})
target_link_libraries(unit_tests lib_cmd)
