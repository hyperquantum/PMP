/*
    Copyright (C) 2020-2022, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_CLIENT_SERVERINTERFACE_H
#define PMP_CLIENT_SERVERINTERFACE_H

#include <QObject>

namespace PMP::Client
{
    class AbstractQueueMonitor;
    class AuthenticationController;
    class CollectionWatcher;
    class CurrentTrackMonitor;
    class DynamicModeController;
    class GeneralController;
    class HistoryController;
    class ScrobblingController;
    class ServerConnection;
    class PlayerController;
    class QueueController;
    class QueueEntryInfoFetcher;
    class QueueEntryInfoStorage;
    class UserDataFetcher;

    class ServerInterface : public QObject
    {
        Q_OBJECT
    public:
        explicit ServerInterface(ServerConnection* connection);

        AuthenticationController& authenticationController();

        GeneralController& generalController();

        PlayerController& playerController();
        CurrentTrackMonitor& currentTrackMonitor();

        QueueController& queueController();
        AbstractQueueMonitor& queueMonitor();
        QueueEntryInfoStorage& queueEntryInfoStorage();
        QueueEntryInfoFetcher& queueEntryInfoFetcher();

        DynamicModeController& dynamicModeController();

        HistoryController& historyController();

        CollectionWatcher& collectionWatcher();
        UserDataFetcher& userDataFetcher();

        ScrobblingController& scrobblingController();

        bool isLoggedIn() const;
        quint32 userLoggedInId() const;
        QString userLoggedInName() const;

        bool connected() const { return _connected; }

    Q_SIGNALS:
        void connectedChanged();

    private:
        ServerConnection* _connection;
        AuthenticationController* _authenticationController { nullptr };
        GeneralController* _generalController { nullptr };
        PlayerController* _simplePlayerController { nullptr };
        CurrentTrackMonitor* _currentTrackMonitor { nullptr };
        QueueController* _queueController { nullptr };
        AbstractQueueMonitor* _queueMonitor { nullptr };
        QueueEntryInfoStorage* _queueEntryInfoStorage { nullptr };
        QueueEntryInfoFetcher* _queueEntryInfoFetcher { nullptr };
        DynamicModeController* _dynamicModeController { nullptr };
        HistoryController* _historyController { nullptr };
        CollectionWatcher* _collectionWatcher { nullptr };
        UserDataFetcher* _userDataFetcher { nullptr };
        ScrobblingController* _scrobblingController { nullptr };
        bool _connected;
    };
}
#endif
