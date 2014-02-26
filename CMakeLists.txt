cmake_minimum_required(VERSION 2.8)
set(CMAKE_LEGACY_CYGWIN_WIN32 0)
project(PMP)

include(FindPkgConfig)

find_package(Qt4 REQUIRED)

pkg_check_modules(TAGLIB REQUIRED taglib>=1.6)

include_directories(${TAGLIB_INCLUDE_DIRS})
link_directories(${TAGLIB_LIBRARY_DIRS})

include(${QT_USE_FILE})

set(PMP_SOURCES main.cpp hashid.cpp)

add_executable(HashTool hash-main.cpp)
add_executable(PMP ${PMP_SOURCES})

target_link_libraries(HashTool ${QT_LIBRARIES} ${TAGLIB_LIBRARIES})
target_link_libraries(PMP ${QT_LIBRARIES} ${TAGLIB_LIBRARIES})
