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

#include "userdatafetcher.h"

#include "collectionwatcher.h"
#include "serverconnection.h"

#include <QTimer>

namespace PMP {

    /* ============================== UserDataFetcher ============================== */

    UserDataFetcher::UserDataFetcher(QObject* parent,
                                     CollectionWatcher* collectionWatcher,
                                     ServerConnection* connection)
     : QObject(parent),
       _collectionWatcher(collectionWatcher),
       _connection(connection)
    {
        //connect(
        //    _connection, &ServerConnection::connected,
        //    this, &UserDataFetcher::connected
        //);
        connect(
            _collectionWatcher, &CollectionWatcher::newTrackReceived,
            this, &UserDataFetcher::onNewTrackReceived
        );
        connect(
            _connection, &ServerConnection::receivedHashUserData,
            this, &UserDataFetcher::receivedHashUserData
        );
    }

    void UserDataFetcher::enableAutoFetchForUser(quint32 userId) {
        auto& userData = _userData[userId];

        if (userData.isAutoFetchEnabled())
            return; /* no change */

        userData.setAutoFetchEnabled(true);

        // TODO : create a new server command to automate this

        /* manual fetch process: iterate through the entire collection and make sure that
                                 each track is fetched */

        QHash<FileHash, CollectionTrackInfo> collection =
                _collectionWatcher->getCollection();

        for (auto it = collection.constBegin(); it != collection.constEnd(); ++it) {
            if (!userData.haveHash(it.key()))
                needToRequestData(userId, it.key());
        }
    }

    //void UserDataFetcher::connected() {
    //    //
    //}

    UserDataFetcher::HashData const* UserDataFetcher::getHashDataForUser(quint32 userId,
                                                                     const FileHash& hash)
    {
        if (hash.isNull()) return nullptr;

        HashData const* hashData = _userData[userId].getHash(hash);
        if (hashData) return hashData;

        needToRequestData(userId, hash);
        return nullptr;
    }

    void UserDataFetcher::onNewTrackReceived(CollectionTrackInfo track) {
        for (auto it = _userData.constBegin(); it != _userData.constEnd(); ++it) {
            auto& userData = it.value();

            if (userData.isAutoFetchEnabled() && !userData.haveHash(track.hash()))
                needToRequestData(it.key(), track.hash());
        }
    }

    void UserDataFetcher::receivedHashUserData(FileHash hash, quint32 userId,
                                               QDateTime previouslyHeard,
                                               qint16 scorePermillage)
    {
        HashData& hashData = _userData[userId].getOrCreateHash(hash);
        hashData.previouslyHeard = previouslyHeard;
        hashData.previouslyHeardReceived = true;
        hashData.scorePermillage = scorePermillage;
        hashData.scoreReceived = true;

        bool first = _pendingNotificationsUsers.isEmpty();

        _pendingNotificationsUsers << userId;

        if (first) {
            QTimer::singleShot(100, this, &UserDataFetcher::sendPendingNotifications);
        }
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

    void UserDataFetcher::sendPendingNotifications() {
        if (_pendingNotificationsUsers.isEmpty()) return;

        Q_FOREACH(quint32 userId, _pendingNotificationsUsers) {
            Q_EMIT dataReceivedForUser(userId);
        }

        _pendingNotificationsUsers.clear();
    }

    void UserDataFetcher::needToRequestData(quint32 userId, const FileHash& hash) {
        bool first = _hashesToFetchForUsers.isEmpty();

        _hashesToFetchForUsers[userId] << hash;

        if (first) {
            QTimer::singleShot(100, this, &UserDataFetcher::sendPendingRequests);
        }
    }
}
