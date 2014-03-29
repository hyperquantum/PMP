cmake_minimum_required(VERSION 2.8.9)
set(CMAKE_LEGACY_CYGWIN_WIN32 0)
project(PMP)

include(FindPkgConfig)

find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Multimedia REQUIRED)
find_package(Qt5Network REQUIRED)

pkg_check_modules(TAGLIB REQUIRED taglib>=1.6)

include_directories(${TAGLIB_INCLUDE_DIRS})
link_directories(${TAGLIB_LIBRARY_DIRS})

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# enable compiler warnings for GCC
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long -pedantic")
endif()

FILE(GLOB PMP_COMMON_SOURCES common/*.cpp)
FILE(GLOB PMP_COMMON_HEADERS common/*.h)
FILE(GLOB PMP_SERVER_SOURCES server/*.cpp)
FILE(GLOB PMP_SERVER_HEADERS server/*.h)

qt5_wrap_cpp(PMP_COMMON_MOCS ${PMP_COMMON_HEADERS})
qt5_wrap_cpp(PMP_SERVER_MOCS ${PMP_SERVER_HEADERS})

add_executable(PMP-HashTool tools/hash-main.cpp)
add_executable(PMP-Cmd-Remote tools/cmd-remote-main.cpp)
add_executable(PMP-Server
    ${PMP_SERVER_SOURCES} ${PMP_SERVER_MOCS}
    ${PMP_COMMON_SOURCES} ${PMP_COMMON_MOCS}
)

qt5_use_modules(PMP-HashTool Core)
qt5_use_modules(PMP-Server Core Multimedia Network)
qt5_use_modules(PMP-Cmd-Remote Core Network)

target_link_libraries(PMP-HashTool ${TAGLIB_LIBRARIES})
target_link_libraries(PMP-Server ${TAGLIB_LIBRARIES})
