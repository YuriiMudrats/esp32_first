# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)
list(APPEND app_sources
    ${CMAKE_CURRENT_LIST_DIR}/../lib/at24c32/at24c32.cpp
)

idf_component_register(
    SRCS ${app_sources}
    INCLUDE_DIRS
        "."
        "../lib/at24c32"
        "../lib/ds1307"
        "../lib/i2c"
        "../lib/logger"
)
