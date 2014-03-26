cmake_minimum_required(VERSION 2.8)
set(CMAKE_LEGACY_CYGWIN_WIN32 0)
project(PMP)

include(FindPkgConfig)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Multimedia REQUIRED)
find_package(Qt5Network REQUIRED)

pkg_check_modules(TAGLIB REQUIRED taglib>=1.6)

include_directories(${TAGLIB_INCLUDE_DIRS})
link_directories(${TAGLIB_LIBRARY_DIRS})

# enable compiler warnings for GCC
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long -pedantic")
endif()

set(PMP_SERVER_SOURCES
    server-main.cpp hashid.cpp filedata.cpp player.cpp server.cpp
    connectedclient.cpp queueentry.cpp
)

add_executable(PMP-HashTool hash-main.cpp)
add_executable(PMP-Server ${PMP_SERVER_SOURCES})
add_executable(PMP-Remote remote-main.cpp)

qt5_use_modules(PMP-HashTool Core)
qt5_use_modules(PMP-Server Core)
qt5_use_modules(PMP-Server Multimedia)
qt5_use_modules(PMP-Server Network)
qt5_use_modules(PMP-Remote Core)
qt5_use_modules(PMP-Remote Network)

target_link_libraries(PMP-HashTool ${TAGLIB_LIBRARIES})
target_link_libraries(PMP-Server ${TAGLIB_LIBRARIES})
