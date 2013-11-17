cmake_minimum_required(VERSION 2.8)
project(PMP)
find_package(Qt4 REQUIRED)
include(${QT_USE_FILE})

set(PMP_SOURCES main.cpp)

add_executable(HashTool hash-main.cpp)
add_executable(PMP ${PMP_SOURCES})

target_link_libraries(HashTool ${QT_LIBRARIES})
target_link_libraries(PMP ${QT_LIBRARIES})
