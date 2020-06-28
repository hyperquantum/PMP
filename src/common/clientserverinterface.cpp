/*
    Copyright (C) 2020, Kevin Andre <hyperquantum@gmail.com>

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

#include "collectionwatcherimpl.h"
#include "serverconnection.h"
#include "simpleplayercontrollerimpl.h"
#include "simpleplayerstatemonitorimpl.h"
#include "userdatafetcher.h"

namespace PMP {

    ClientServerInterface::ClientServerInterface(QObject* parent,
                                                 ServerConnection* connection)
     : QObject(parent), _connection(connection),
       _simplePlayerController(nullptr),
       _simplePlayerStateMonitor(nullptr),
       _collectionWatcher(nullptr),
       _userDataFetcher(nullptr)
    {
        //
    }

    SimplePlayerController& ClientServerInterface::simplePlayerController()
    {
        if (!_simplePlayerController)
            _simplePlayerController = new SimplePlayerControllerImpl(_connection);

        return *_simplePlayerController;
    }

    SimplePlayerStateMonitor& ClientServerInterface::simplePlayerStateMonitor()
    {
        if (!_simplePlayerStateMonitor)
            _simplePlayerStateMonitor = new SimplePlayerStateMonitorImpl(_connection);

        return *_simplePlayerStateMonitor;
    }

    CollectionWatcher& ClientServerInterface::collectionWatcher()
    {
        if (!_collectionWatcher)
            _collectionWatcher = new CollectionWatcherImpl(_connection);

        return *_collectionWatcher;
    }

    UserDataFetcher& ClientServerInterface::getUserDataFetcher()
    {
        if (_userDataFetcher == nullptr) {
            _userDataFetcher = new UserDataFetcher(this, _connection);
        }

        return *_userDataFetcher;
    }
}
