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
    class LocalHashIdRepository;
    class PlayerController;
    class QueueController;
    class QueueEntryInfoFetcher;
    class QueueEntryInfoStorage;
    class ScrobblingController;
    class ServerConnection;
    class UserDataFetcher;

    class ServerInterface : public QObject
    {
        Q_OBJECT
    protected:
        ServerInterface() {}
    public:
        ~ServerInterface() {}

        virtual LocalHashIdRepository* hashIdRepository() const = 0;

        virtual AuthenticationController& authenticationController() = 0;

        virtual GeneralController& generalController() = 0;

        virtual PlayerController& playerController() = 0;
        virtual CurrentTrackMonitor& currentTrackMonitor() = 0;

        virtual QueueController& queueController() = 0;
        virtual AbstractQueueMonitor& queueMonitor() = 0;
        virtual QueueEntryInfoStorage& queueEntryInfoStorage() = 0;
        virtual QueueEntryInfoFetcher& queueEntryInfoFetcher() = 0;

        virtual DynamicModeController& dynamicModeController() = 0;

        virtual HistoryController& historyController() = 0;

        virtual CollectionWatcher& collectionWatcher() = 0;
        virtual UserDataFetcher& userDataFetcher() = 0;

        virtual ScrobblingController& scrobblingController() = 0;

        virtual bool isLoggedIn() const = 0;
        virtual quint32 userLoggedInId() const = 0;
        virtual QString userLoggedInName() const = 0;

        virtual bool connected() const = 0;

    Q_SIGNALS:
        void connectedChanged();
    };

    class ServerInterfaceImpl : public ServerInterface
    {
        Q_OBJECT
    public:
        explicit ServerInterfaceImpl(ServerConnection* connection);

        LocalHashIdRepository* hashIdRepository() const override;

        AuthenticationController& authenticationController() override;

        GeneralController& generalController() override;

        PlayerController& playerController() override;
        CurrentTrackMonitor& currentTrackMonitor() override;

        QueueController& queueController() override;
        AbstractQueueMonitor& queueMonitor() override;
        QueueEntryInfoStorage& queueEntryInfoStorage() override;
        QueueEntryInfoFetcher& queueEntryInfoFetcher() override;

        DynamicModeController& dynamicModeController() override;

        HistoryController& historyController() override;

        CollectionWatcher& collectionWatcher() override;
        UserDataFetcher& userDataFetcher() override;

        ScrobblingController& scrobblingController() override;

        bool isLoggedIn() const override;
        quint32 userLoggedInId() const override;
        QString userLoggedInName() const override;

        bool connected() const override { return _connected; }

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
