/*
    Copyright (C) 2016-2020, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_USERDATAFETCHER_H
#define PMP_USERDATAFETCHER_H

#include "common/filehash.h"

#include <QDateTime>
#include <QHash>
#include <QObject>
#include <QPair>
#include <QSet>

namespace PMP {

    class ServerConnection;

    class UserDataFetcher : public QObject {
        Q_OBJECT
    public:
        struct HashData;

        UserDataFetcher(QObject* parent, ServerConnection* connection);

        HashData const* getHashDataForUser(quint32 userId, FileHash const& hash);

    Q_SIGNALS:
        void dataReceivedForUser(quint32 userId);

    private slots:
        //void connected();
        void receivedHashUserData(FileHash hash, quint32 userId,
                                  QDateTime previouslyHeard, qint16 scorePermillage);
        void sendPendingRequests();
        void sendPendingNotifications();

    private:
        class UserData;

        UserData* getOrCreateUserData(quint32 userId);
        void needToRequestData(quint32 userId, FileHash const& hash);

        ServerConnection* _connection;
        QHash<quint32, UserData*> _userData;
        QHash<quint32, QSet<FileHash> > _hashesToFetchForUsers;
        QSet<quint32> _pendingNotificationsUsers;
    };

    struct UserDataFetcher::HashData {
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

    class UserDataFetcher::UserData {
    public:
        UserData(quint32 userId)
         : _user(userId)
        {
            //
        }

        HashData& getOrCreateHash(FileHash const& hash) { return _hashes[hash]; }

        HashData const* getHash(FileHash const& hash) const {
            auto it = _hashes.find(hash);
            if (it == _hashes.constEnd()) return nullptr;

            return &it.value();
        }

    private:
        quint32 _user;
        QHash<FileHash, HashData> _hashes;
    };
}
#endif
