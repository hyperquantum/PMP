/*
    Copyright (C) 2014-2023, Kevin Andre <hyperquantum@gmail.com>

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

#include "queueentryinfostorageimpl.h"

#include "common/containerutil.h"

#include "serverconnection.h"

namespace PMP::Client
{
    QueueEntryInfoStorageImpl::QueueEntryInfoStorageImpl(ServerConnection* connection)
     : QueueEntryInfoStorage(connection),
       _connection(connection)
    {
        connect(
            _connection, &ServerConnection::connected,
            this, &QueueEntryInfoStorageImpl::connected
        );
        connect(
            _connection, &ServerConnection::disconnected,
            this, &QueueEntryInfoStorageImpl::connectionBroken
        );
        connect(
            _connection, &ServerConnection::receivedQueueEntryHash,
            this, &QueueEntryInfoStorageImpl::receivedQueueEntryHash
        );
        connect(
            _connection, &ServerConnection::receivedTrackInfo,
            this, &QueueEntryInfoStorageImpl::receivedTrackInfo
        );
        connect(
            _connection, &ServerConnection::receivedPossibleFilenames,
            this, &QueueEntryInfoStorageImpl::receivedPossibleFilenames
        );
    }

    void QueueEntryInfoStorageImpl::dropInfoFor(quint32 queueId)
    {
        _infoRequestsSent.remove(queueId);
        _hashRequestsSent.remove(queueId);

        if (!_entries.contains(queueId)) return;

        delete _entries[queueId];
        _entries.remove(queueId);
    }

    QueueEntryInfo* QueueEntryInfoStorageImpl::entryInfoByQueueId(quint32 queueId)
    {
        if (queueId == 0)
            return nullptr;

        QueueEntryInfo* info = _entries[queueId];
        if (!info)
        {
            qDebug() << "QueueEntryInfoStorageImpl: requesting info/hash for QID"
                     << queueId;
            sendInfoRequest(queueId);
        }
        else if (info->hashId().isZero()
                 && !info->isTrack().isFalse()
                 && !_hashRequestsSent.contains(queueId))
        {
            qDebug() << "QueueEntryInfoStorageImpl: requesting hash for QID"
                     << queueId;
            sendHashRequest(queueId);
        }

        return info;
    }

    void QueueEntryInfoStorageImpl::fetchEntry(quint32 queueId)
    {
        (void)entryInfoByQueueId(queueId);
    }

    void QueueEntryInfoStorageImpl::fetchEntries(QList<quint32> queueIds)
    {
        QList<quint32> idsToFetch;
        idsToFetch.reserve(queueIds.size());

        for (auto queueId : queueIds)
        {
            if (!_entries.contains(queueId))
            {
                idsToFetch.append(queueId);
                _entries[queueId] = new QueueEntryInfo(queueId);
            }

            _infoRequestsSent << queueId;
            _hashRequestsSent << queueId;
        }

        qDebug() << "QueueEntryInfoStorageImpl: requesting info/hash for"
                 << idsToFetch.size() << "QIDs";

        _connection->sendQueueEntryInfoRequest(idsToFetch);
        _connection->sendQueueEntryHashRequest(idsToFetch);
    }

    void QueueEntryInfoStorageImpl::refetchEntries(QList<quint32> queueIds)
    {
        qDebug() << "QueueEntryInfoStorageImpl: re-requesting info/hash for"
                 << queueIds.size() << "QIDs";

        ContainerUtil::addToSet(queueIds, _infoRequestsSent);
        ContainerUtil::addToSet(queueIds, _hashRequestsSent);

        _connection->sendQueueEntryInfoRequest(queueIds);
        _connection->sendQueueEntryHashRequest(queueIds);
    }

    void QueueEntryInfoStorageImpl::connected()
    {
        //
    }

    void QueueEntryInfoStorageImpl::connectionBroken()
    {
        _infoRequestsSent.clear();
        _hashRequestsSent.clear();

        qDeleteAll(_entries); /* delete all objects before clearing */
        _entries.clear();
    }

    void QueueEntryInfoStorageImpl::receivedQueueEntryHash(quint32 queueID,
                                                           QueueEntryType type,
                                                           LocalHashId hashId)
    {
        qDebug() << "QueueEntryInfoStorageImpl: received hash for QID" << queueID << ":"
                 << hashId;

        _hashRequestsSent.remove(queueID);

        QueueEntryInfo*& info = _entries[queueID];

        if (!info)
        {
            info = new QueueEntryInfo(queueID);
        }
        else
        {
            if (info->type() == type && info->hashId() == hashId)
            {
                return; /* no change */
            }
        }

        info->setHash(type, hashId);

        enqueueTrackChangeNotification(queueID);
    }

    void QueueEntryInfoStorageImpl::receivedTrackInfo(quint32 queueID,
                                                      QueueEntryType type,
                                                      qint64 lengthMilliseconds,
                                                      QString title,
                                                      QString artist)
    {
        qDebug() << "QueueEntryInfoStorageImpl: received info for QID" << queueID << ":"
                 << "title:" << title << " artist:" << artist;

        _infoRequestsSent.remove(queueID);

        QueueEntryInfo*& info = _entries[queueID];

        if (!info)
        {
            info = new QueueEntryInfo(queueID);
        }
        else
        {
            if (info->type() == type
                    && info->lengthInMilliseconds() == lengthMilliseconds
                    && info->artist() == artist
                    && info->title() == title)
            {
                return; /* no change */
            }
        }

        info->setInfo(type, lengthMilliseconds, title, artist);

        if (info->needFilename())
        {
            /* no title/artist info available, so we want to display a filename instead */
            _connection->sendPossibleFilenamesRequest(queueID);
        }

        enqueueTrackChangeNotification(queueID);
    }

    void QueueEntryInfoStorageImpl::receivedPossibleFilenames(quint32 queueID,
                                                              QList<QString> names)
    {
        qDebug() << "QueueEntryInfoStorageImpl: received possible filenames for QID"
                 << queueID;

        QueueEntryInfo*& info = _entries[queueID];

        if (!info)
        {
            info = new QueueEntryInfo(queueID);
        }

        bool changed = info->setPossibleFilenames(names);

        if (!changed) return;

        enqueueTrackChangeNotification(queueID);
    }

    void QueueEntryInfoStorageImpl::enqueueTrackChangeNotification(quint32 queueID)
    {
        bool first = _trackChangeNotificationsPending.isEmpty();

        _trackChangeNotificationsPending << queueID;

        if (first)
        {
            QTimer::singleShot(50, this,
                               &QueueEntryInfoStorageImpl::emitTracksChangedSignal);
        }
    }

    void QueueEntryInfoStorageImpl::emitTracksChangedSignal()
    {
        if (_trackChangeNotificationsPending.isEmpty()) return;

        QList<quint32> list = _trackChangeNotificationsPending.values();
        _trackChangeNotificationsPending.clear();

        qDebug() << "QueueEntryInfoStorageImpl: going to emit tracksChanged signal for"
                 << list.size() << "tracks";
        Q_EMIT tracksChanged(list);
    }

    void QueueEntryInfoStorageImpl::sendInfoRequest(quint32 queueId)
    {
        sendHashRequest(queueId);

        if (_infoRequestsSent.contains(queueId))
            return; /* sent already and waiting for an answer */

        _connection->sendQueueEntryInfoRequest(queueId);
        _infoRequestsSent << queueId;
    }

    void QueueEntryInfoStorageImpl::sendHashRequest(quint32 queueId)
    {
        if (_hashRequestsSent.contains(queueId)) return;

        QList<uint> ids;
        ids.append(queueId);
        _connection->sendQueueEntryHashRequest(ids);

        _hashRequestsSent << queueId;
    }
}
