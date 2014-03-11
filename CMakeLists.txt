cmake_minimum_required(VERSION 2.8)
set(CMAKE_LEGACY_CYGWIN_WIN32 0)
project(PMP)

include(FindPkgConfig)

find_package(Qt5Core REQUIRED)
find_package(Qt5Multimedia REQUIRED)

pkg_check_modules(TAGLIB REQUIRED taglib>=1.6)

include_directories(${TAGLIB_INCLUDE_DIRS})
link_directories(${TAGLIB_LIBRARY_DIRS})

# enable compiler warnings for GCC
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long -pedantic")
endif()

set(PMP_SOURCES main.cpp hashid.cpp filedata.cpp)

add_executable(HashTool hash-main.cpp)
add_executable(PMP ${PMP_SOURCES})

qt5_use_modules(HashTool Core)
qt5_use_modules(PMP Core)
qt5_use_modules(PMP Multimedia)

target_link_libraries(HashTool ${TAGLIB_LIBRARIES})
target_link_libraries(PMP ${TAGLIB_LIBRARIES})
