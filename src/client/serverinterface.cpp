/*
    Copyright (C) 2020-2023, Kevin Andre <hyperquantum@gmail.com>

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

#include "serverinterface.h"

#include "authenticationcontrollerimpl.h"
#include "collectionwatcherimpl.h"
#include "currenttrackmonitorimpl.h"
#include "dynamicmodecontrollerimpl.h"
#include "generalcontrollerimpl.h"
#include "historycontrollerimpl.h"
#include "playercontrollerimpl.h"
#include "queuecontrollerimpl.h"
#include "queueentryinfofetcher.h"
#include "queueentryinfostorageimpl.h"
#include "queuemonitor.h"
#include "serverconnection.h"
#include "userdatafetcher.h"

namespace PMP::Client
{
    ServerInterface::ServerInterface(ServerConnection* connection)
     : QObject(connection),
       _connection(connection),
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

    LocalHashIdRepository* ServerInterface::hashIdRepository() const
    {
        return _connection->hashIdRepository();
    }

    AuthenticationController& ServerInterface::authenticationController()
    {
        if (!_authenticationController)
            _authenticationController = new AuthenticationControllerImpl(_connection);

        return *_authenticationController;
    }

    GeneralController& ServerInterface::generalController()
    {
        if (!_generalController)
            _generalController = new GeneralControllerImpl(_connection);

        return *_generalController;
    }

    PlayerController& ServerInterface::playerController()
    {
        if (!_simplePlayerController)
            _simplePlayerController = new PlayerControllerImpl(_connection);

        return *_simplePlayerController;
    }

    CurrentTrackMonitor& ServerInterface::currentTrackMonitor()
    {
        if (!_currentTrackMonitor)
            _currentTrackMonitor = new CurrentTrackMonitorImpl(&queueEntryInfoStorage(),
                                                               _connection);

        return *_currentTrackMonitor;
    }

    QueueController& ServerInterface::queueController()
    {
        if (!_queueController)
            _queueController = new QueueControllerImpl(_connection);

        return *_queueController;
    }

    AbstractQueueMonitor& ServerInterface::queueMonitor()
    {
        if (!_queueMonitor)
            _queueMonitor = new QueueMonitor(_connection);

        return *_queueMonitor;
    }

    QueueEntryInfoStorage& ServerInterface::queueEntryInfoStorage()
    {
        if (!_queueEntryInfoStorage)
            _queueEntryInfoStorage = new QueueEntryInfoStorageImpl(_connection);

        return *_queueEntryInfoStorage;
    }

    QueueEntryInfoFetcher& ServerInterface::queueEntryInfoFetcher()
    {
        if (!_queueEntryInfoFetcher)
        {
            _queueEntryInfoFetcher = new QueueEntryInfoFetcher(this, &queueMonitor(),
                                                               &queueEntryInfoStorage(),
                                                               _connection);
        }

        return *_queueEntryInfoFetcher;
    }

    DynamicModeController& ServerInterface::dynamicModeController()
    {
        if (!_dynamicModeController)
            _dynamicModeController = new DynamicModeControllerImpl(_connection);

        return *_dynamicModeController;
    }

    HistoryController& ServerInterface::historyController()
    {
        if (!_historyController)
            _historyController = new HistoryControllerImpl(_connection);

        return *_historyController;
    }

    CollectionWatcher& ServerInterface::collectionWatcher()
    {
        if (!_collectionWatcher)
            _collectionWatcher = new CollectionWatcherImpl(_connection);

        return *_collectionWatcher;
    }

    UserDataFetcher& ServerInterface::userDataFetcher()
    {
        if (!_userDataFetcher)
        {
            _userDataFetcher =
                    new UserDataFetcher(this, &collectionWatcher(), _connection);
        }

        return *_userDataFetcher;
    }

    bool ServerInterface::isLoggedIn() const
    {
        return _connection->isLoggedIn();
    }

    quint32 ServerInterface::userLoggedInId() const
    {
        return _connection->userLoggedInId();
    }

    QString ServerInterface::userLoggedInName() const
    {
        return _connection->userLoggedInName();
    }
}
