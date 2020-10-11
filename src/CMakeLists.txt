# Copyright (C) 2011-2020, Kevin Andre <hyperquantum@gmail.com>
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
    common/clientserverinterface.cpp
    common/collectionwatcherimpl.cpp
    common/commonmetatypes.cpp
    common/currenttrackmonitorimpl.cpp
    common/dynamicmodecontrollerimpl.cpp
    common/fileanalyzer.cpp
    common/filehash.cpp
    common/logging.cpp
    common/networkprotocol.cpp
    common/networkutil.cpp
    common/obfuscator.cpp
    common/playercontrollerimpl.cpp
    common/scrobblerstatus.cpp
    common/scrobblingprovider.cpp
    common/serverconnection.cpp
    common/serverdiscoverer.cpp
    common/tagdata.cpp
    common/tribool.cpp
    common/userdatafetcher.cpp
    common/util.cpp
)
set(PMP_COMMON_HEADERS
    common/clientserverinterface.h
    common/collectionfetcher.h
    common/collectionwatcher.h
    common/collectionwatcherimpl.h
    common/currenttrackmonitor.h
    common/currenttrackmonitorimpl.h
    common/dynamicmodecontroller.h
    common/dynamicmodecontrollerimpl.h
    common/playercontroller.h
    common/playercontrollerimpl.h
    common/serverconnection.h
    common/serverdiscoverer.h
    common/userdatafetcher.h
)

set(PMP_SERVER_SOURCES
    server/addtohistorytask.cpp
    server/clientrequestorigin.cpp
    server/collectionmonitor.cpp
    server/connectedclient.cpp
    server/database.cpp
    server/generator.cpp
    server/history.cpp
    server/lastfmscrobblingbackend.cpp
    server/lastfmscrobblingdataprovider.cpp
    server/player.cpp
    server/playerqueue.cpp
    server/preloader.cpp
    server/queueentry.cpp
    server/randomtrackssource.cpp
    server/resolver.cpp
    server/scrobbler.cpp
    server/scrobbling.cpp
    server/scrobblingbackend.cpp
    server/scrobblingdataprovider.cpp
    server/scrobblinghost.cpp
    server/selftest.cpp
    server/server.cpp
    server/serverhealthmonitor.cpp
    server/serverinterface.cpp
    server/servermetatypes.cpp
    server/serversettings.cpp
    server/tokenencoder.cpp
    server/tracktoscrobble.cpp
    server/userdataforhashesfetcher.cpp
    server/userdatafortracksfetcher.cpp
    server/users.cpp
)
set(PMP_SERVER_HEADERS
    server/addtohistorytask.h
    server/collectionmonitor.h
    server/connectedclient.h
    server/generator.h
    server/history.h
    server/lastfmscrobblingbackend.h
    server/player.h
    server/playerqueue.h
    server/preloader.h
    server/queueentry.h
    server/randomtrackssource.h
    server/resolver.h
    server/scrobbler.h
    server/scrobbling.h
    server/scrobblingbackend.h
    server/scrobblinghost.h
    server/server.h
    server/serverhealthmonitor.h
    server/serverinterface.h
    server/serversettings.h
    server/userdataforhashesfetcher.h
    server/userdatafortracksfetcher.h
    server/users.h
)

set(PMP_GUI_REMOTE_SOURCES
    gui-remote/abstractqueuemonitor.cpp
    gui-remote/autopersonalmodeaction.cpp
    gui-remote/collectiontablemodel.cpp
    gui-remote/collectionwidget.cpp
    gui-remote/colors.cpp
    gui-remote/colorswitcher.cpp
    gui-remote/connectionwidget.cpp
    gui-remote/gui-remote-metatypes.cpp
    gui-remote/loginwidget.cpp
    gui-remote/mainwidget.cpp
    gui-remote/mainwindow.cpp
    gui-remote/playerhistorymodel.cpp
    gui-remote/powermanagement.cpp
    gui-remote/precisetrackprogressmonitor.cpp
    gui-remote/queueentryinfofetcher.cpp
    gui-remote/queuemediator.cpp
    gui-remote/queuemodel.cpp
    gui-remote/queuemonitor.cpp
    gui-remote/scoreformatdelegate.cpp
    gui-remote/trackinfodialog.cpp
    gui-remote/trackprogresswidget.cpp
    gui-remote/useraccountcreationwidget.cpp
    gui-remote/userpickerwidget.cpp
)
set(PMP_GUI_REMOTE_HEADERS
    gui-remote/abstractqueuemonitor.h
    gui-remote/autopersonalmodeaction.h
    gui-remote/collectiontablemodel.h
    gui-remote/collectionwidget.h
    gui-remote/colorswitcher.h
    gui-remote/connectionwidget.h
    gui-remote/loginwidget.h
    gui-remote/mainwidget.h
    gui-remote/mainwindow.h
    gui-remote/playerhistorymodel.h
    gui-remote/powermanagement.h
    gui-remote/precisetrackprogressmonitor.h
    gui-remote/queueentryinfofetcher.h
    gui-remote/queuemediator.h
    gui-remote/queuemodel.h
    gui-remote/queuemonitor.h
    gui-remote/scoreformatdelegate.h
    gui-remote/trackinfodialog.h
    gui-remote/trackprogresswidget.h
    gui-remote/useraccountcreationwidget.h
    gui-remote/userpickerwidget.h
)
set(PMP_GUI_REMOTE_UIS
    gui-remote/collectionwidget.ui
    gui-remote/connectionwidget.ui
    gui-remote/loginwidget.ui
    gui-remote/mainwidget.ui
    gui-remote/trackinfodialog.ui
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

configure_file(common/version.h.in common/version.h)

if (NOT ENABLE_WIN32_CONSOLE)
  set(PMP_WIN32_FLAG WIN32)
endif (NOT ENABLE_WIN32_CONSOLE)

add_executable(PMP-HashTool
    tools/hash-main.cpp
    common/audiodata.cpp
    common/commonmetatypes.cpp
    common/fileanalyzer.cpp
    common/filehash.cpp
    common/tagdata.cpp
)
add_executable(PMP-Cmd-Remote tools/cmd-remote-main.cpp)
add_executable(quicktest
    tools/quicktest.cpp
    ${PMP_SERVER_SOURCES} ${PMP_SERVER_MOCS}
    ${PMP_COMMON_SOURCES} ${PMP_COMMON_MOCS}
)

add_executable(PMP-Server
    server/server-main.cpp
    ${PMP_SERVER_SOURCES} ${PMP_SERVER_MOCS}
    ${PMP_COMMON_SOURCES} ${PMP_COMMON_MOCS}
)
add_executable(PMP-GUI-Remote
    ${PMP_WIN32_FLAG}
    gui-remote/gui-remote-main.cpp
    ${PMP_GUI_REMOTE_SOURCES} ${PMP_GUI_REMOTE_MOCS}
    ${PMP_GUI_REMOTE_UICS}
    ${PMP_GUI_REMOTE_QRCS}
    ${PMP_COMMON_SOURCES} ${PMP_COMMON_MOCS}
)

# Qt dependencies for each executable
target_link_libraries(PMP-HashTool Qt5::Core)
target_link_libraries(quicktest Qt5::Core Qt5::Multimedia Qt5::Network Qt5::Sql Qt5::Xml)
target_link_libraries(PMP-Server Qt5::Core Qt5::Multimedia Qt5::Network Qt5::Sql Qt5::Xml)
target_link_libraries(PMP-Cmd-Remote Qt5::Core Qt5::Network)
target_link_libraries(PMP-GUI-Remote Qt5::Core Qt5::Gui Qt5::Network Qt5::Widgets)

# Link executables to TagLib
target_link_libraries(PMP-HashTool ${TAGLIB_LIBRARIES})
target_link_libraries(quicktest ${TAGLIB_LIBRARIES})
target_link_libraries(PMP-Server ${TAGLIB_LIBRARIES})
target_link_libraries(PMP-GUI-Remote ${TAGLIB_LIBRARIES})
