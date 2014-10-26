cmake_minimum_required(VERSION 2.8.9)
set(CMAKE_LEGACY_CYGWIN_WIN32 0)
project(PMP)

include(FindPkgConfig)

find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Multimedia REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Qt5Sql REQUIRED)
find_package(Qt5Widgets REQUIRED)

pkg_check_modules(TAGLIB REQUIRED taglib>=1.6)

include_directories(${TAGLIB_INCLUDE_DIRS})
link_directories(${TAGLIB_LIBRARY_DIRS})

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# enable compiler warnings for GCC
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long -pedantic")
endif()

set(PMP_COMMON_SOURCES
    common/audiodata.cpp
    common/filedata.cpp
    common/hashid.cpp
    common/networkutil.cpp
    common/serverconnection.cpp
    common/tagdata.cpp
)
set(PMP_COMMON_HEADERS
    common/serverconnection.h
)

set(PMP_SERVER_SOURCES
    server/connectedclient.cpp
    server/fileanalysistask.cpp
    server/generator.cpp
    server/history.cpp
    server/player.cpp
    server/queue.cpp
    server/queueentry.cpp
    server/resolver.cpp
    server/server.cpp
    server/server-main.cpp
)
set(PMP_SERVER_HEADERS
    server/connectedclient.h
    server/fileanalysistask.h
    server/generator.h
    server/history.h
    server/player.h
    server/queue.h
    server/queueentry.h
    server/resolver.h
    server/server.h
)

set(PMP_GUI_REMOTE_SOURCES
    gui-remote/connectionwidget.cpp
    gui-remote/gui-remote-main.cpp
    gui-remote/mainwidget.cpp
    gui-remote/mainwindow.cpp
    gui-remote/queuemodel.cpp
    gui-remote/queuemonitor.cpp
)
set(PMP_GUI_REMOTE_HEADERS
    gui-remote/connectionwidget.h
    gui-remote/mainwidget.h
    gui-remote/mainwindow.h
    gui-remote/queuemodel.h
    gui-remote/queuemonitor.h
)
set(PMP_GUI_REMOTE_UIS
    gui-remote/connectionwidget.ui
    gui-remote/mainwidget.ui
)

qt5_wrap_cpp(PMP_COMMON_MOCS ${PMP_COMMON_HEADERS})
qt5_wrap_cpp(PMP_SERVER_MOCS ${PMP_SERVER_HEADERS})
qt5_wrap_cpp(PMP_GUI_REMOTE_MOCS ${PMP_GUI_REMOTE_HEADERS})
qt5_wrap_ui(PMP_GUI_REMOTE_UICS ${PMP_GUI_REMOTE_UIS})

add_executable(PMP-HashTool
    tools/hash-main.cpp
    common/audiodata.cpp
    common/filedata.cpp
    common/hashid.cpp
    common/tagdata.cpp
)
add_executable(PMP-Cmd-Remote tools/cmd-remote-main.cpp)

add_executable(PMP-Server
    ${PMP_SERVER_SOURCES} ${PMP_SERVER_MOCS}
    ${PMP_COMMON_SOURCES} ${PMP_COMMON_MOCS}
)
add_executable(PMP-GUI-Remote
    ${PMP_GUI_REMOTE_SOURCES} ${PMP_GUI_REMOTE_MOCS}
    ${PMP_GUI_REMOTE_UICS}
    ${PMP_COMMON_SOURCES} ${PMP_COMMON_MOCS}
)

# Qt dependencies for each executable
qt5_use_modules(PMP-HashTool Core)
qt5_use_modules(PMP-Server Core Multimedia Network Sql)
qt5_use_modules(PMP-Cmd-Remote Core Network)
qt5_use_modules(PMP-GUI-Remote Core Gui Network Widgets)

# Link executables to TagLib
target_link_libraries(PMP-HashTool ${TAGLIB_LIBRARIES})
target_link_libraries(PMP-Server ${TAGLIB_LIBRARIES})
target_link_libraries(PMP-GUI-Remote ${TAGLIB_LIBRARIES})
