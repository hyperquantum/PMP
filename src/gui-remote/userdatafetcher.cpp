/*
    Copyright (C) 2016, Kevin Andre <hyperquantum@gmail.com>

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

#include "userdatafetcher.h"

#include "common/serverconnection.h"

#include <QTimer>

namespace PMP {

    /* ============================== UserDataFetcher ============================== */

    UserDataFetcher::UserDataFetcher(QObject* parent, ServerConnection* connection)
     : QObject(parent), _connection(connection)
    {
        //connect(
        //    _connection, &ServerConnection::connected,
        //    this, &UserDataFetcher::connected
        //);
        connect(
            _connection, &ServerConnection::receivedHashUserData,
            this, &UserDataFetcher::receivedHashUserData
        );
    }

    //void UserDataFetcher::connected() {
    //    //
    //}

    UserDataFetcher::HashData const* UserDataFetcher::getHashDataForUser(quint32 userId,
                                                                     const FileHash& hash)
    {
        UserData* userData = getOrCreateUserData(userId);
        HashData const* hashData = userData->getHash(hash);

        if (hashData) return hashData;

        needToRequestData(userId, hash);
        return nullptr;
    }

    void UserDataFetcher::receivedHashUserData(FileHash hash, quint32 userId,
                                               QDateTime previouslyHeard)
    {
        UserData* userData = getOrCreateUserData(userId);

        HashData& hashData = userData->getOrCreateHash(hash);
        hashData.previouslyHeard = previouslyHeard;
        hashData.previouslyHeardKnown = previouslyHeard.isValid();
    }

    void UserDataFetcher::sendPendingRequests() {
        if (_hashesToFetchForUsers.isEmpty()) return;

        Q_FOREACH(quint32 userId, _hashesToFetchForUsers.keys()) {
            _connection->sendHashUserDataRequest(
                userId, _hashesToFetchForUsers.value(userId).toList()
            );
        }

        _hashesToFetchForUsers.clear();
    }

    UserDataFetcher::UserData* UserDataFetcher::getOrCreateUserData(quint32 userId)
    {
        UserData* data = _userData.value(userId, nullptr);
        if (data) return data;

        data = new UserData(userId);
        _userData.insert(userId, data);
        return data;
    }

    void UserDataFetcher::needToRequestData(quint32 userId, const FileHash& hash) {
        bool first = _hashesToFetchForUsers.isEmpty();

        _hashesToFetchForUsers[userId] << hash;

        if (first) {
            QTimer::singleShot(100, this, SLOT(sendPendingRequests()));
        }
    }
}
