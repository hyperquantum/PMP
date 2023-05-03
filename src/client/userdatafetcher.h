/*
    Copyright (C) 2016-2023, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_CLIENT_USERDATAFETCHER_H
#define PMP_CLIENT_USERDATAFETCHER_H

#include "collectiontrackinfo.h"

#include <QDateTime>
#include <QHash>
#include <QObject>
#include <QPair>
#include <QSet>

namespace PMP::Client
{
    class CollectionWatcher;
    class ServerConnection;

    class UserDataFetcher : public QObject
    {
        Q_OBJECT
    public:
        struct HashData;

        UserDataFetcher(QObject* parent, CollectionWatcher* collectionWatcher,
                        ServerConnection* connection);

        void enableAutoFetchForUser(quint32 userId);

        HashData const* getHashDataForUser(quint32 userId, LocalHashId hashId);

    Q_SIGNALS:
        void dataReceivedForUser(quint32 userId);
        void userTrackDataChanged(quint32 userId, LocalHashId hashId);

    private Q_SLOTS:
        //void connected();
        void onNewTrackReceived(CollectionTrackInfo track);
        void receivedHashUserData(LocalHashId hashId, quint32 userId,
                                  QDateTime previouslyHeard, qint16 scorePermillage);
        void sendPendingRequests();
        void sendPendingNotifications();

    private:
        class UserData
        {
        public:
            UserData()
             : _autoFetchEnabled(false)
            {
                //
            }

            bool isAutoFetchEnabled() const { return _autoFetchEnabled; }

            void setAutoFetchEnabled(bool newValue)
            {
                _autoFetchEnabled = newValue;
            }

            HashData& getOrCreateHash(LocalHashId hashId) { return _hashes[hashId]; }

            bool haveHash(LocalHashId hashId) const
            {
                return _hashes.contains(hashId);
            }

            HashData const* getHash(LocalHashId hashId) const
            {
                auto it = _hashes.find(hashId);
                if (it == _hashes.constEnd()) return nullptr;

                return &it.value();
            }

        private:
            QHash<LocalHashId, HashData> _hashes;
            bool _autoFetchEnabled;
        };

        void needToRequestData(quint32 userId, LocalHashId hashId);

        CollectionWatcher* _collectionWatcher;
        ServerConnection* _connection;
        QHash<quint32, UserData> _userData;
        QHash<quint32, QSet<LocalHashId>> _hashesToFetchForUsers;
        QSet<quint32> _pendingNotificationsUsers;
    };

    struct UserDataFetcher::HashData
    {
        HashData()
         : previouslyHeardReceived(false), scoreReceived(false)
        {
            //
        }

        bool previouslyHeardReceived;
        QDateTime previouslyHeard;

        bool scoreReceived;
        qint16 scorePermillage;
    };
}
#endif
