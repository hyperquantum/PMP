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

#include "userdatafetcher.h"

#include "collectionwatcher.h"
#include "serverconnection.h"

#include <QtDebug>
#include <QTimer>

namespace PMP::Client
{
    /* ============================== UserDataFetcher ============================== */

    UserDataFetcherImpl::UserDataFetcherImpl(QObject* parent,
                                             CollectionWatcher* collectionWatcher,
                                             ServerConnection* connection)
     : UserDataFetcher(parent),
       _collectionWatcher(collectionWatcher),
       _connection(connection)
    {
        //connect(
        //    _connection, &ServerConnection::connected,
        //    this, &UserDataFetcherImpl::connected
        //);
        connect(
            _collectionWatcher, &CollectionWatcher::newTrackReceived,
            this, &UserDataFetcherImpl::onNewTrackReceived
        );
        connect(
            _connection, &ServerConnection::receivedHashUserData,
            this, &UserDataFetcherImpl::receivedHashUserData
        );
    }

    void UserDataFetcherImpl::enableAutoFetchForUser(quint32 userId)
    {
        auto& userData = _userData[userId];

        if (userData.isAutoFetchEnabled())
            return; /* no change */

        qDebug() << "UserDataFetcher: enabling auto fetch for user" << userId;

        userData.setAutoFetchEnabled(true);

        // TODO : create a new server command to automate this

        /* manual fetch process: iterate through the entire collection and make sure that
                                 each track is fetched */

        _collectionWatcher->enableCollectionDownloading();
        auto collection = _collectionWatcher->getCollection();

        for (auto it = collection.constBegin(); it != collection.constEnd(); ++it)
        {
            if (!userData.haveHash(it.key()))
                needToRequestData(userId, it.key());
        }
    }

    //void UserDataFetcherImpl::connected()
    //{
    //    //
    //}

    UserDataFetcher::HashData const* UserDataFetcherImpl::getHashDataForUser(
                                                                       quint32 userId,
                                                                       LocalHashId hashId)
    {
        if (hashId.isZero())
            return nullptr;

        HashData const* hashData = _userData[userId].getHash(hashId);
        if (hashData) return hashData;

        needToRequestData(userId, hashId);
        return nullptr;
    }

    void UserDataFetcherImpl::onNewTrackReceived(CollectionTrackInfo track)
    {
        for (auto it = _userData.constBegin(); it != _userData.constEnd(); ++it)
        {
            auto userId = it.key();
            auto& userData = it.value();

            if (userData.isAutoFetchEnabled() && !userData.haveHash(track.hashId()))
                needToRequestData(userId, track.hashId());
        }
    }

    void UserDataFetcherImpl::receivedHashUserData(LocalHashId hashId, quint32 userId,
                                                   QDateTime previouslyHeard,
                                                   qint16 scorePermillage)
    {
        HashData& hashData = _userData[userId].getOrCreateHash(hashId);
        hashData.previouslyHeard = previouslyHeard;
        hashData.previouslyHeardReceived = true;
        hashData.scorePermillage = scorePermillage;
        hashData.scoreReceived = true;

        bool first = _pendingNotificationsUsers.isEmpty();

        _pendingNotificationsUsers << userId;

        if (first)
        {
            QTimer::singleShot(100, this, &UserDataFetcherImpl::sendPendingNotifications);
        }

        Q_EMIT userTrackDataChanged(userId, hashId);
    }

    void UserDataFetcherImpl::sendPendingRequests()
    {
        if (_hashesToFetchForUsers.isEmpty()) return;

        for (quint32 userId : _hashesToFetchForUsers.keys())
        {
            _connection->sendHashUserDataRequest(
                userId, _hashesToFetchForUsers.value(userId).values()
            );
        }

        _hashesToFetchForUsers.clear();
    }

    void UserDataFetcherImpl::sendPendingNotifications()
    {
        if (_pendingNotificationsUsers.isEmpty()) return;

        for (quint32 userId : qAsConst(_pendingNotificationsUsers))
        {
            Q_EMIT dataReceivedForUser(userId);
        }

        _pendingNotificationsUsers.clear();
    }

    void UserDataFetcherImpl::needToRequestData(quint32 userId, LocalHashId hashId)
    {
        bool first = _hashesToFetchForUsers.isEmpty();

        _hashesToFetchForUsers[userId] << hashId;

        if (first)
        {
            QTimer::singleShot(100, this, &UserDataFetcherImpl::sendPendingRequests);
        }
    }
}
