# Copyright (C) 2011-2023, Kevin Andre <hyperquantum@gmail.com>
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
    common/networkutil.cpp
    common/powermanagement.cpp
    common/startstopeventstatus.cpp
    common/tagdata.cpp
    common/tribool.cpp
    common/util.cpp
)
set(PMP_COMMON_HEADERS
    common/powermanagement.h
)

set(PMP_CLIENT_SOURCES
    client/authenticationcontrollerimpl.cpp
    client/clientmetatypes.cpp
    client/collectionwatcherimpl.cpp
    client/compatibilityinterfacecontrollerimpl.cpp
    client/compatibilityinterfaceimpl.cpp
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
    client/compatibilityinterface.h
    client/compatibilityinterfacecontroller.h
    client/compatibilityinterfacecontrollerimpl.h
    client/compatibilityinterfaceimpl.h
    client/compatibilityinterfaceviewcreator.h
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
    client/serverconnection.h
    client/serverdiscoverer.h
    client/serverinterface.h
    client/userdatafetcher.h
)

set(PMP_SERVER_SOURCES
    server/analyzer.cpp
    server/collectionmonitor.cpp
    server/compatibilityuicontroller.cpp
    server/compatibilityuicontrollercollection.cpp
    server/compatibilityuicontrollers.cpp
    server/connectedclient.cpp
    server/database.cpp
    server/delayedstart.cpp
    server/dynamicmodecriteria.cpp
    server/dynamictrackgenerator.cpp
    server/generator.cpp
    server/hashidregistrar.cpp
    server/hashrelations.cpp
    server/history.cpp
    server/historystatistics.cpp
    server/historystatisticsprefetcher.cpp
    server/player.cpp
    server/playerqueue.cpp
    server/preloader.cpp
    server/queueentry.cpp
    server/randomtrackssource.cpp
    server/resolver.cpp
    server/serverhealthmonitor.cpp
    server/serverinterface.cpp
    server/servermetatypes.cpp
    server/serversettings.cpp
    server/tcpserver.cpp
    server/trackgeneratorbase.cpp
    server/trackrepetitionchecker.cpp
    server/trackstats.cpp
    server/users.cpp
    server/wavetrackgenerator.cpp
)
set(PMP_SERVER_HEADERS
    server/analyzer.h
    server/collectionmonitor.h
    server/compatibilityuicontroller.h
    server/compatibilityuicontrollercollection.h
    server/compatibilityuicontrollers.h
    server/connectedclient.h
    server/delayedstart.h
    server/dynamictrackgenerator.h
    server/generator.h
    server/history.h
    server/historystatistics.h
    server/historystatisticsprefetcher.h
    server/player.h
    server/playerqueue.h
    server/preloader.h
    server/randomtrackssource.h
    server/resolver.h
    server/serverhealthmonitor.h
    server/serverinterface.h
    server/serversettings.h
    server/tcpserver.h
    server/trackgeneratorbase.h
    server/trackrepetitionchecker.h
    server/users.h
    server/wavetrackgenerator.h
)

set(PMP_CMD_REMOTE_SOURCES
    cmd-remote/administrativecommands.cpp
    cmd-remote/commandbase.cpp
    cmd-remote/commandlineclient.cpp
    cmd-remote/commandparser.cpp
    cmd-remote/console.cpp
    cmd-remote/miscellaneouscommands.cpp
    cmd-remote/playercommands.cpp
    cmd-remote/queuecommands.cpp
)
set(PMP_CMD_REMOTE_HEADERS
    cmd-remote/administrativecommands.h
    cmd-remote/command.h
    cmd-remote/commandbase.h
    cmd-remote/commandlineclient.h
    cmd-remote/miscellaneouscommands.h
    cmd-remote/playercommands.h
    cmd-remote/queuecommands.h
)

set(PMP_GUI_REMOTE_SOURCES
    gui-remote/autopersonalmodeaction.cpp
    gui-remote/clickablelabel.cpp
    gui-remote/collectiontablemodel.cpp
    gui-remote/collectionwidget.cpp
    gui-remote/colors.cpp
    gui-remote/colorswitcher.cpp
    gui-remote/compatibilityinterfaceview.cpp
    gui-remote/connectionwidget.cpp
    gui-remote/delayedstartdialog.cpp
    gui-remote/delayedstartnotification.cpp
    gui-remote/dynamicmodeparametersdialog.cpp
    gui-remote/gui-remote-metatypes.cpp
    gui-remote/loginwidget.cpp
    gui-remote/mainwidget.cpp
    gui-remote/mainwindow.cpp
    gui-remote/notificationbar.cpp
    gui-remote/playerhistorymodel.cpp
    gui-remote/precisetrackprogressmonitor.cpp
    gui-remote/queuemediator.cpp
    gui-remote/queuemodel.cpp
    gui-remote/scoreformatdelegate.cpp
    gui-remote/trackinfodialog.cpp
    gui-remote/trackjudge.cpp
    gui-remote/trackprogresswidget.cpp
    gui-remote/useraccountcreationwidget.cpp
    gui-remote/userpickerwidget.cpp
    gui-remote/waitingspinnerwidget.cpp
)
set(PMP_GUI_REMOTE_HEADERS
    gui-remote/autopersonalmodeaction.h
    gui-remote/clickablelabel.h
    gui-remote/collectiontablemodel.h
    gui-remote/collectionwidget.h
    gui-remote/colorswitcher.h
    gui-remote/compatibilityinterfaceview.h
    gui-remote/connectionwidget.h
    gui-remote/delayedstartdialog.h
    gui-remote/delayedstartnotification.h
    gui-remote/dynamicmodeparametersdialog.h
    gui-remote/loginwidget.h
    gui-remote/mainwidget.h
    gui-remote/mainwindow.h
    gui-remote/notificationbar.h
    gui-remote/playerhistorymodel.h
    gui-remote/precisetrackprogressmonitor.h
    gui-remote/queuemediator.h
    gui-remote/queuemodel.h
    gui-remote/scoreformatdelegate.h
    gui-remote/trackinfodialog.h
    gui-remote/trackprogresswidget.h
    gui-remote/useraccountcreationwidget.h
    gui-remote/userpickerwidget.h
    gui-remote/waitingspinnerwidget.h
)
set(PMP_GUI_REMOTE_UIS
    gui-remote/collectionwidget.ui
    gui-remote/connectionwidget.ui
    gui-remote/delayedstartdialog.ui
    gui-remote/dynamicmodeparametersdialog.ui
    gui-remote/loginwidget.ui
    gui-remote/mainwidget.ui
    gui-remote/notificationbar.ui
    gui-remote/trackinfodialog.ui
    gui-remote/useraccountcreationwidget.ui
    gui-remote/userpickerwidget.ui
)
set(PMP_GUI_REMOTE_RESOURCES
    gui-remote/mainwindow.qrc
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

add_executable(PMP-GUI-Remote
    ${PMP_WIN32_FLAG}
    gui-remote/gui-remote-main.cpp
    ${PMP_GUI_REMOTE_SOURCES} ${PMP_GUI_REMOTE_MOCS}
    ${PMP_GUI_REMOTE_UICS}
    ${PMP_GUI_REMOTE_QRCS}
)
target_link_libraries(PMP-GUI-Remote
    $<TARGET_OBJECTS:PmpClient>
    $<TARGET_OBJECTS:PmpCommon>
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
target_link_libraries(PMP-Cmd-Remote ${TAGLIB_LIBRARIES})
target_link_libraries(PMP-GUI-Remote ${TAGLIB_LIBRARIES})
