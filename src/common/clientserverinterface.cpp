/*
    Copyright (C) 2020-2021, Kevin Andre <hyperquantum@gmail.com>

    This file is part of PMP (Party Music Player).

    PMP is free software: you can redistribute it and/or modify it under the
    terms of the GNU General Public License as published by the Free Software
    Foundation, either version 3 of the License, or (at your option) any later
    version.

    PMP is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
    details.

    You should have received a copy of the GNU General Public License along
    with PMP.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "clientserverinterface.h"

#include "authenticationcontrollerimpl.h"
#include "collectionwatcherimpl.h"
#include "compatibilityinterfacecontrollerimpl.h"
#include "currenttrackmonitorimpl.h"
#include "dynamicmodecontrollerimpl.h"
#include "generalcontrollerimpl.h"
#include "playercontrollerimpl.h"
#include "queuecontrollerimpl.h"
#include "queueentryinfofetcher.h"
#include "queuemonitor.h"
#include "serverconnection.h"
#include "userdatafetcher.h"

namespace PMP
{
    ClientServerInterface::ClientServerInterface(ServerConnection* connection)
     : QObject(connection),
       _connection(connection),
       _authenticationController(nullptr),
       _generalController(nullptr),
       _simplePlayerController(nullptr),
       _currentTrackMonitor(nullptr),
       _queueController(nullptr),
       _queueMonitor(nullptr),
       _queueEntryInfoFetcher(nullptr),
       _dynamicModeController(nullptr),
       _collectionWatcher(nullptr),
       _userDataFetcher(nullptr),
       _compatibilityUiController(nullptr),
       _connected(connection->isConnected())
    {
        connect(
            connection, &ServerConnection::connected,
            this,
            [this]()
            {
                if (_connected) return;

                _connected = true;
                Q_EMIT connectedChanged();
            },
            Qt::QueuedConnection
        );
        connect(
            connection, &ServerConnection::disconnected,
            this,
            [this]()
            {
                qDebug() << "connection has been disconnected";

                if (!_connected) return;

                _connected = false;
                Q_EMIT connectedChanged();
            },
            Qt::QueuedConnection
        );
    }

    AuthenticationController& ClientServerInterface::authenticationController()
    {
        if (!_authenticationController)
            _authenticationController = new AuthenticationControllerImpl(_connection);

        return *_authenticationController;
    }

    GeneralController& ClientServerInterface::generalController()
    {
        if (!_generalController)
            _generalController = new GeneralControllerImpl(_connection);

        return *_generalController;
    }

    PlayerController& ClientServerInterface::playerController()
    {
        if (!_simplePlayerController)
            _simplePlayerController = new PlayerControllerImpl(_connection);

        return *_simplePlayerController;
    }

    CurrentTrackMonitor& ClientServerInterface::currentTrackMonitor()
    {
        if (!_currentTrackMonitor)
            _currentTrackMonitor = new CurrentTrackMonitorImpl(_connection);

        return *_currentTrackMonitor;
    }

    QueueController& ClientServerInterface::queueController()
    {
        if (!_queueController)
            _queueController = new QueueControllerImpl(_connection);

        return *_queueController;
    }

    AbstractQueueMonitor& ClientServerInterface::queueMonitor()
    {
        if (!_queueMonitor)
            _queueMonitor = new QueueMonitor(_connection);

        return *_queueMonitor;
    }

    QueueEntryInfoFetcher& ClientServerInterface::queueEntryInfoFetcher()
    {
        if (!_queueEntryInfoFetcher)
        {
            _queueEntryInfoFetcher =
                    new QueueEntryInfoFetcher(this, &queueMonitor(), _connection);
        }

        return *_queueEntryInfoFetcher;
    }

    DynamicModeController& ClientServerInterface::dynamicModeController()
    {
        if (!_dynamicModeController)
            _dynamicModeController = new DynamicModeControllerImpl(_connection);

        return *_dynamicModeController;
    }

    CollectionWatcher& ClientServerInterface::collectionWatcher()
    {
        if (!_collectionWatcher)
            _collectionWatcher = new CollectionWatcherImpl(_connection);

        return *_collectionWatcher;
    }

    UserDataFetcher& ClientServerInterface::userDataFetcher()
    {
        if (!_userDataFetcher)
        {
            _userDataFetcher =
                    new UserDataFetcher(this, &collectionWatcher(), _connection);
        }

        return *_userDataFetcher;
    }

    CompatibilityInterfaceController* ClientServerInterface::compatibilityUiController()
    {
        if (!_compatibilityUiController)
        {
            _compatibilityUiController =
                    new CompatibilityInterfaceControllerImpl(_connection,
                                                          UserInterfaceLanguage::English);
        }

        return _compatibilityUiController;
    }

    bool ClientServerInterface::isLoggedIn() const
    {
        return _connection->isLoggedIn();
    }

    quint32 ClientServerInterface::userLoggedInId() const
    {
        return _connection->userLoggedInId();
    }

    QString ClientServerInterface::userLoggedInName() const
    {
        return _connection->userLoggedInName();
    }
}
