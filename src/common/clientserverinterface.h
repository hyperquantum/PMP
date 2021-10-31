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

#ifndef PMP_CLIENTSERVERINTERFACE_H
#define PMP_CLIENTSERVERINTERFACE_H

#include <QObject>

namespace PMP
{
    class AbstractQueueMonitor;
    class CollectionWatcher;
    class CurrentTrackMonitor;
    class DynamicModeController;
    class GeneralController;
    class ServerConnection;
    class PlayerController;
    class QueueController;
    class QueueEntryInfoFetcher;
    class UserDataFetcher;

    class ClientServerInterface : public QObject
    {
        Q_OBJECT
    public:
        ClientServerInterface(ServerConnection* connection);

        GeneralController& generalController();

        PlayerController& playerController();
        CurrentTrackMonitor& currentTrackMonitor();

        QueueController& queueController();
        AbstractQueueMonitor& queueMonitor();
        QueueEntryInfoFetcher& queueEntryInfoFetcher();

        DynamicModeController& dynamicModeController();

        CollectionWatcher& collectionWatcher();
        UserDataFetcher& userDataFetcher();

        bool isLoggedIn() const;
        quint32 userLoggedInId() const;
        QString userLoggedInName() const;

        bool connected() const { return _connected; }

    Q_SIGNALS:
        void connectedChanged();

    private:
        ServerConnection* _connection;
        GeneralController* _generalController;
        PlayerController* _simplePlayerController;
        CurrentTrackMonitor* _currentTrackMonitor;
        QueueController* _queueController;
        AbstractQueueMonitor* _queueMonitor;
        QueueEntryInfoFetcher* _queueEntryInfoFetcher;
        DynamicModeController* _dynamicModeController;
        CollectionWatcher* _collectionWatcher;
        UserDataFetcher* _userDataFetcher;
        bool _connected;
    };
}
#endif
