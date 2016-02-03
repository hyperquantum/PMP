# Copyright (C) 2011-2016, Kevin Andre <hyperquantum@gmail.com>
#
# This file is part of PMP (Party Music Player).
#
# PMP is free software: you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# PMP is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along
# with PMP.  If not, see <http://www.gnu.org/licenses/>.

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(PMP_COMMON_SOURCES
    common/audiodata.cpp
    common/collectiontrackinfo.cpp
    common/filedata.cpp
    common/filehash.cpp
    common/networkprotocol.cpp
    common/networkutil.cpp
    common/serverconnection.cpp
    common/serverdiscoverer.cpp
    common/tagdata.cpp
    common/util.cpp
)
set(PMP_COMMON_HEADERS
    common/serverconnection.h
    common/serverdiscoverer.h
)

set(PMP_SERVER_SOURCES
    server/collectionmonitor.cpp
    server/connectedclient.cpp
    server/database.cpp
    server/generator.cpp
    server/history.cpp
    server/player.cpp
    server/queue.cpp
    server/queueentry.cpp
    server/resolver.cpp
    server/server.cpp
    server/server-main.cpp
    server/serversettings.cpp
    server/users.cpp
)
set(PMP_SERVER_HEADERS
    server/collectionmonitor.h
    server/connectedclient.h
    server/generator.h
    server/history.h
    server/player.h
    server/queue.h
    server/queueentry.h
    server/resolver.h
    server/server.h
    server/serversettings.h
    server/users.h
)

set(PMP_GUI_REMOTE_SOURCES
    gui-remote/abstractqueuemonitor.cpp
    gui-remote/autopersonalmodeaction.cpp
    gui-remote/collectiontablemodel.cpp
    gui-remote/collectionwidget.cpp
    gui-remote/connectionwidget.cpp
    gui-remote/currenttrackmonitor.cpp
    gui-remote/gui-remote-main.cpp
    gui-remote/loginwidget.cpp
    gui-remote/mainwidget.cpp
    gui-remote/mainwindow.cpp
    gui-remote/queueentryinfofetcher.cpp
    gui-remote/queuemediator.cpp
    gui-remote/queuemodel.cpp
    gui-remote/queuemonitor.cpp
    gui-remote/trackprogresswidget.cpp
    gui-remote/useraccountcreationwidget.cpp
    gui-remote/userpickerwidget.cpp
)
set(PMP_GUI_REMOTE_HEADERS
    gui-remote/abstractqueuemonitor.h
    gui-remote/autopersonalmodeaction.h
    gui-remote/collectiontablemodel.h
    gui-remote/collectionwidget.h
    gui-remote/connectionwidget.h
    gui-remote/currenttrackmonitor.h
    gui-remote/loginwidget.h
    gui-remote/mainwidget.h
    gui-remote/mainwindow.h
    gui-remote/queueentryinfofetcher.h
    gui-remote/queuemediator.h
    gui-remote/queuemodel.h
    gui-remote/queuemonitor.h
    gui-remote/trackprogresswidget.h
    gui-remote/useraccountcreationwidget.h
    gui-remote/userpickerwidget.h
)
set(PMP_GUI_REMOTE_UIS
    gui-remote/collectionwidget.ui
    gui-remote/connectionwidget.ui
    gui-remote/loginwidget.ui
    gui-remote/mainwidget.ui
    gui-remote/useraccountcreationwidget.ui
    gui-remote/userpickerwidget.ui
)
set(PMP_GUI_REMOTE_RESOURCES
    gui-remote/mainwindow.qrc
)

qt5_wrap_cpp(PMP_COMMON_MOCS ${PMP_COMMON_HEADERS})
qt5_wrap_cpp(PMP_SERVER_MOCS ${PMP_SERVER_HEADERS})
qt5_wrap_cpp(PMP_GUI_REMOTE_MOCS ${PMP_GUI_REMOTE_HEADERS})
qt5_wrap_ui(PMP_GUI_REMOTE_UICS ${PMP_GUI_REMOTE_UIS})
qt5_add_resources(PMP_GUI_REMOTE_QRCS ${PMP_GUI_REMOTE_RESOURCES})

add_executable(PMP-HashTool
    tools/hash-main.cpp
    common/audiodata.cpp
    common/filedata.cpp
    common/filehash.cpp
    common/tagdata.cpp
)
add_executable(PMP-Cmd-Remote tools/cmd-remote-main.cpp)
add_executable(quicktest
    tools/quicktest.cpp
    common/util.cpp
)

add_executable(PMP-Server
    ${PMP_SERVER_SOURCES} ${PMP_SERVER_MOCS}
    ${PMP_COMMON_SOURCES} ${PMP_COMMON_MOCS}
)
add_executable(PMP-GUI-Remote
    ${PMP_GUI_REMOTE_SOURCES} ${PMP_GUI_REMOTE_MOCS}
    ${PMP_GUI_REMOTE_UICS}
    ${PMP_GUI_REMOTE_QRCS}
    ${PMP_COMMON_SOURCES} ${PMP_COMMON_MOCS}
)

# Qt dependencies for each executable
qt5_use_modules(PMP-HashTool Core)
qt5_use_modules(quicktest Core Network)
qt5_use_modules(PMP-Server Core Multimedia Network Sql)
qt5_use_modules(PMP-Cmd-Remote Core Network)
qt5_use_modules(PMP-GUI-Remote Core Gui Network Widgets)

# Link executables to TagLib
target_link_libraries(PMP-HashTool ${TAGLIB_LIBRARIES})
target_link_libraries(PMP-Server ${TAGLIB_LIBRARIES})
target_link_libraries(PMP-GUI-Remote ${TAGLIB_LIBRARIES})
