# Copyright (C) 2011-2024, Kevin Andre <hyperquantum@gmail.com>
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
    common/commonmetatypes.cpp
    common/fileanalyzer.cpp
    common/filehash.cpp
    common/logging.cpp
    common/networkprotocol.cpp
    common/networkprotocolextensions.cpp
    common/networkutil.cpp
    common/obfuscator.cpp
    common/passwordstrengthevaluator.cpp
    common/powermanagement.cpp
    common/scrobblerstatus.cpp
    common/scrobblingprovider.cpp
    common/startstopeventstatus.cpp
    common/tagdata.cpp
    common/tribool.cpp
    common/util.cpp
    common/versioninfo.cpp
)
set(PMP_COMMON_HEADERS
    common/powermanagement.h
)

set(PMP_CLIENT_SOURCES
    client/authenticationcontrollerimpl.cpp
    client/clientmetatypes.cpp
    client/collectionwatcherimpl.cpp
    client/currenttrackmonitorimpl.cpp
    client/dynamicmodecontrollerimpl.cpp
    client/generalcontrollerimpl.cpp
    client/historycontrollerimpl.cpp
    client/localhashidrepository.cpp
    client/playercontrollerimpl.cpp
    client/queuecontrollerimpl.cpp
    client/queueentryinfofetcher.cpp
    client/queueentryinfostorage.cpp
    client/queueentryinfostorageimpl.cpp
    client/queuehashesmonitor.cpp
    client/queuemonitor.cpp
    client/scrobblingcontrollerimpl.cpp
    client/servercapabilitiesimpl.cpp
    client/serverconnection.cpp
    client/serverdiscoverer.cpp
    client/serverinterface.cpp
    client/userdatafetcher.cpp
)
set(PMP_CLIENT_HEADERS
    client/abstractqueuemonitor.h
    client/authenticationcontroller.h
    client/authenticationcontrollerimpl.h
    client/collectionfetcher.h
    client/collectionwatcher.h
    client/collectionwatcherimpl.h
    client/currenttrackmonitor.h
    client/currenttrackmonitorimpl.h
    client/dynamicmodecontroller.h
    client/dynamicmodecontrollerimpl.h
    client/generalcontroller.h
    client/generalcontrollerimpl.h
    client/historycontroller.h
    client/historycontrollerimpl.h
    client/playercontroller.h
    client/playercontrollerimpl.h
    client/queuecontroller.h
    client/queuecontrollerimpl.h
    client/queueentryinfofetcher.h
    client/queueentryinfostorage.h
    client/queueentryinfostorageimpl.h
    client/queuehashesmonitor.h
    client/queuemonitor.h
    client/scrobblingcontroller.h
    client/scrobblingcontrollerimpl.h
    client/serverconnection.h
    client/serverdiscoverer.h
    client/serverinterface.h
    client/userdatafetcher.h
)

set(PMP_SERVER_SOURCES
    server/analyzer.cpp
    server/collectionmonitor.cpp
    server/connectedclient.cpp
    server/database.cpp
    server/delayedstart.cpp
    server/dynamicmodecriteria.cpp
    server/dynamictrackgenerator.cpp
    server/filefinder.cpp
    server/filelocations.cpp
    server/generator.cpp
    server/hashidregistrar.cpp
    server/hashrelations.cpp
    server/history.cpp
    server/historystatistics.cpp
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
    server/scrobblinghost.cpp
    server/selftest.cpp
    server/serverhealthmonitor.cpp
    server/serverinterface.cpp
    server/servermetatypes.cpp
    server/serversettings.cpp
    server/tcpserver.cpp
    server/tokenencoder.cpp
    server/trackgeneratorbase.cpp
    server/trackinfoproviderimpl.cpp
    server/trackrepetitionchecker.cpp
    server/trackstats.cpp
    server/userhashstatscache.cpp
    server/userhashstatscachefixer.cpp
    server/users.cpp
    server/wavetrackgenerator.cpp
)
set(PMP_SERVER_HEADERS
    server/analyzer.h
    server/collectionmonitor.h
    server/connectedclient.h
    server/delayedstart.h
    server/dynamictrackgenerator.h
    server/filefinder.h
    server/generator.h
    server/history.h
    server/historystatistics.h
    server/lastfmscrobblingbackend.h
    server/player.h
    server/playerqueue.h
    server/preloader.h
    server/randomtrackssource.h
    server/resolver.h
    server/scrobbler.h
    server/scrobbling.h
    server/scrobblingbackend.h
    server/scrobblinghost.h
    server/serverhealthmonitor.h
    server/serverinterface.h
    server/serversettings.h
    server/tcpserver.h
    server/trackgeneratorbase.h
    server/trackrepetitionchecker.h
    server/userhashstatscachefixer.h
    server/users.h
    server/wavetrackgenerator.h
)

set(PMP_CMD_REMOTE_SOURCES
    cmd-remote/administrativecommands.cpp
    cmd-remote/commandbase.cpp
    cmd-remote/commandlineclient.cpp
    cmd-remote/commandparser.cpp
    cmd-remote/console.cpp
    cmd-remote/historycommands.cpp
    cmd-remote/miscellaneouscommands.cpp
    cmd-remote/playercommands.cpp
    cmd-remote/queuecommands.cpp
    cmd-remote/scrobblingcommands.cpp
)
set(PMP_CMD_REMOTE_HEADERS
    cmd-remote/administrativecommands.h
    cmd-remote/command.h
    cmd-remote/commandbase.h
    cmd-remote/commandlineclient.h
    cmd-remote/historycommands.h
    cmd-remote/miscellaneouscommands.h
    cmd-remote/playercommands.h
    cmd-remote/queuecommands.h
    cmd-remote/scrobblingcommands.h
)

set(PMP_GUI_REMOTE_SOURCES
    desktop-remote/autopersonalmodeaction.cpp
    desktop-remote/clickablelabel.cpp
    desktop-remote/collectiontablemodel.cpp
    desktop-remote/collectionwidget.cpp
    desktop-remote/colors.cpp
    desktop-remote/colorswitcher.cpp
    desktop-remote/connectionwidget.cpp
    desktop-remote/delayedstartdialog.cpp
    desktop-remote/delayedstartnotification.cpp
    desktop-remote/dynamicmodeparametersdialog.cpp
    desktop-remote/desktop-remote-metatypes.cpp
    desktop-remote/historymodel.cpp
    desktop-remote/loginwidget.cpp
    desktop-remote/mainwidget.cpp
    desktop-remote/mainwindow.cpp
    desktop-remote/notificationbar.cpp
    desktop-remote/playerhistorymodel.cpp
    desktop-remote/precisetrackprogressmonitor.cpp
    desktop-remote/queuemediator.cpp
    desktop-remote/queuemodel.cpp
    desktop-remote/scoreformatdelegate.cpp
    desktop-remote/scrobblingauthenticationdialog.cpp
    desktop-remote/trackinfodialog.cpp
    desktop-remote/trackjudge.cpp
    desktop-remote/trackprogresswidget.cpp
    desktop-remote/useraccountcreationwidget.cpp
    desktop-remote/userforstatisticsdisplay.cpp
    desktop-remote/userpickerwidget.cpp
    desktop-remote/userscrobblingdialog.cpp
    desktop-remote/waitingspinnerwidget.cpp
)
set(PMP_GUI_REMOTE_HEADERS
    desktop-remote/autopersonalmodeaction.h
    desktop-remote/clickablelabel.h
    desktop-remote/collectiontablemodel.h
    desktop-remote/collectionwidget.h
    desktop-remote/colorswitcher.h
    desktop-remote/connectionwidget.h
    desktop-remote/delayedstartdialog.h
    desktop-remote/delayedstartnotification.h
    desktop-remote/dynamicmodeparametersdialog.h
    desktop-remote/historymodel.h
    desktop-remote/loginwidget.h
    desktop-remote/mainwidget.h
    desktop-remote/mainwindow.h
    desktop-remote/notificationbar.h
    desktop-remote/playerhistorymodel.h
    desktop-remote/precisetrackprogressmonitor.h
    desktop-remote/queuemediator.h
    desktop-remote/queuemodel.h
    desktop-remote/scoreformatdelegate.h
    desktop-remote/scrobblingauthenticationdialog.h
    desktop-remote/trackinfodialog.h
    desktop-remote/trackprogresswidget.h
    desktop-remote/useraccountcreationwidget.h
    desktop-remote/userforstatisticsdisplay.h
    desktop-remote/userpickerwidget.h
    desktop-remote/userscrobblingdialog.h
    desktop-remote/waitingspinnerwidget.h
)
set(PMP_GUI_REMOTE_UIS
    desktop-remote/collectionwidget.ui
    desktop-remote/connectionwidget.ui
    desktop-remote/delayedstartdialog.ui
    desktop-remote/dynamicmodeparametersdialog.ui
    desktop-remote/loginwidget.ui
    desktop-remote/mainwidget.ui
    desktop-remote/notificationbar.ui
    desktop-remote/scrobblingauthenticationdialog.ui
    desktop-remote/trackinfodialog.ui
    desktop-remote/useraccountcreationwidget.ui
    desktop-remote/userpickerwidget.ui
    desktop-remote/userscrobblingdialog.ui
)
set(PMP_GUI_REMOTE_RESOURCES
    desktop-remote/mainwindow.qrc
)

qt5_wrap_cpp(PMP_COMMON_MOCS ${PMP_COMMON_HEADERS})
qt5_wrap_cpp(PMP_CLIENT_MOCS ${PMP_CLIENT_HEADERS})
qt5_wrap_cpp(PMP_SERVER_MOCS ${PMP_SERVER_HEADERS})
qt5_wrap_cpp(PMP_CMD_REMOTE_MOCS ${PMP_CMD_REMOTE_HEADERS})
qt5_wrap_cpp(PMP_GUI_REMOTE_MOCS ${PMP_GUI_REMOTE_HEADERS})
qt5_wrap_ui(PMP_GUI_REMOTE_UICS ${PMP_GUI_REMOTE_UIS})
qt5_add_resources(PMP_GUI_REMOTE_QRCS ${PMP_GUI_REMOTE_RESOURCES})

configure_file(common/version.h.in common/version.h)

if (NOT ENABLE_WIN32_CONSOLE)
  set(PMP_WIN32_FLAG WIN32)
endif (NOT ENABLE_WIN32_CONSOLE)

add_library(PmpCommon OBJECT ${PMP_COMMON_SOURCES} ${PMP_COMMON_MOCS})
target_link_libraries(PmpCommon Qt5::Core)

add_library(PmpClient OBJECT ${PMP_CLIENT_SOURCES} ${PMP_CLIENT_MOCS})
target_link_libraries(PmpClient Qt5::Core Qt5::Network)

add_library(PmpCmdRemote OBJECT ${PMP_CMD_REMOTE_SOURCES} ${PMP_CMD_REMOTE_MOCS})
target_link_libraries(PmpCmdRemote Qt5::Core Qt5::Network)

add_library(PmpGuiRemote OBJECT
    ${PMP_GUI_REMOTE_SOURCES} ${PMP_GUI_REMOTE_MOCS}
    ${PMP_GUI_REMOTE_UICS} ${PMP_GUI_REMOTE_QRCS}
)
target_link_libraries(PmpGuiRemote Qt5::Core Qt5::Gui Qt5::Network Qt5::Widgets)

add_library(PmpServer OBJECT ${PMP_SERVER_SOURCES} ${PMP_SERVER_MOCS})
target_link_libraries(PmpServer Qt5::Core Qt5::Multimedia Qt5::Network Qt5::Sql Qt5::Xml)

add_executable(PMP-HashTool
    tools/hash-main.cpp
)
target_link_libraries(PMP-HashTool
    $<TARGET_OBJECTS:PmpCommon>
)

add_executable(quicktest
    tools/quicktest.cpp
)
target_link_libraries(quicktest
    $<TARGET_OBJECTS:PmpServer>
    $<TARGET_OBJECTS:PmpClient>
    $<TARGET_OBJECTS:PmpCommon>
)

add_executable(PMP-Server
    server/server-main.cpp
)
target_link_libraries(PMP-Server
    $<TARGET_OBJECTS:PmpServer>
    $<TARGET_OBJECTS:PmpCommon>
)

add_executable(PMP-Cmd-Remote
    cmd-remote/cmd-remote-main.cpp
)
target_link_libraries(PMP-Cmd-Remote
    $<TARGET_OBJECTS:PmpCmdRemote>
    $<TARGET_OBJECTS:PmpClient>
    $<TARGET_OBJECTS:PmpCommon>
)

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set (GUI_REMOTE_WIN32_RESOURCES desktop-remote/winres.rc)
endif()

add_executable(PMP-Desktop-Remote
    ${PMP_WIN32_FLAG}
    ${GUI_REMOTE_WIN32_RESOURCES}
    desktop-remote/desktop-remote-main.cpp

)
target_link_libraries(PMP-Desktop-Remote
    $<TARGET_OBJECTS:PmpGuiRemote>
    $<TARGET_OBJECTS:PmpClient>
    $<TARGET_OBJECTS:PmpCommon>
)

# Qt dependencies for each executable
target_link_libraries(PMP-HashTool Qt5::Core)
target_link_libraries(quicktest Qt5::Core Qt5::Multimedia Qt5::Network Qt5::Sql Qt5::Xml)
target_link_libraries(PMP-Server Qt5::Core Qt5::Multimedia Qt5::Network Qt5::Sql Qt5::Xml)
target_link_libraries(PMP-Cmd-Remote Qt5::Core Qt5::Network)
target_link_libraries(PMP-Desktop-Remote Qt5::Core Qt5::Gui Qt5::Network Qt5::Widgets)

# Link executables to TagLib
target_link_libraries(PMP-HashTool ${TAGLIB_LIBRARIES})
target_link_libraries(quicktest ${TAGLIB_LIBRARIES})
target_link_libraries(PMP-Server ${TAGLIB_LIBRARIES})
target_link_libraries(PMP-Cmd-Remote ${TAGLIB_LIBRARIES})
target_link_libraries(PMP-Desktop-Remote ${TAGLIB_LIBRARIES})
