/*
    Copyright (C) 2014-2020, Kevin Andre <hyperquantum@gmail.com>

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

#include "queueentryinfofetcher.h"

#include "common/serverconnection.h"

#include <QtDebug>
#include <QTimer>

namespace PMP {

    /* ========================== QueueEntryInfo ========================== */

    QueueEntryInfo::QueueEntryInfo(quint32 queueID)
     : _queueID(queueID), _type(QueueEntryType::Unknown), _lengthSeconds(-1)
    {
        //
    }

    void QueueEntryInfo::setHash(QueueEntryType type, const FileHash& hash) {
        _type = type;
        _hash = hash;
    }

    void QueueEntryInfo::setInfo(QueueEntryType type, int lengthInSeconds,
                                 QString const& title, QString const& artist)
    {
        _type = type;
        _lengthSeconds = lengthInSeconds;
        _title = title;
        _artist = artist;
    }

    bool QueueEntryInfo::setPossibleFilenames(QList<QString> const& names) {
        if (names.empty()) return false;

        int shortestLength = names[0].length();
        int longestLength = names[0].length();

        int limit = 20;
        Q_FOREACH(QString name, names) {
            if (name.length() < shortestLength) shortestLength = name.length();
            else if (name.length() > longestLength) longestLength = name.length();

            if (--limit <= 0) break;
        }

        /* Avoid a potential overflow, don't add shortest and longest, the result does not
           need to be exact.
           Also, if there are only two possibilities, we favor the longest. */
        int middleLength = (shortestLength + 1) / 2 + (longestLength + 1) / 2 + 1;

        QString middle = names[0];

        limit = 10;
        Q_FOREACH(QString name, names) {
            int diff = std::abs(name.length() - middleLength);
            int oldDiff = std::abs(middle.length() - middleLength);

            if (diff < oldDiff) middle = name;

            if (--limit <= 0) break;
        }

        if (_informativeFilename.trimmed() == "" && _informativeFilename != middle) {
            _informativeFilename = middle;
            return true;
        }

        return false;
    }

    /* ========================== QueueEntryInfoFetcher ========================== */

    QueueEntryInfoFetcher::QueueEntryInfoFetcher(QObject* parent,
                                                 AbstractQueueMonitor* monitor,
                                                 ServerConnection* connection)
     : QObject(parent), _monitor(monitor), _connection(connection), _userPlayingFor(0)
    {
        connect(
            _connection, &ServerConnection::connected,
            this, &QueueEntryInfoFetcher::connected
        );
        connect(
            _connection, &ServerConnection::receivedUserPlayingFor,
            this, &QueueEntryInfoFetcher::receivedUserPlayingFor
        );
        connect(
            _connection, &ServerConnection::receivedQueueEntryHash,
            this, &QueueEntryInfoFetcher::receivedQueueEntryHash
        );
        connect(
            _connection, &ServerConnection::receivedTrackInfo,
            this, &QueueEntryInfoFetcher::receivedTrackInfo
        );
        connect(
            _connection, &ServerConnection::receivedPossibleFilenames,
            this, &QueueEntryInfoFetcher::receivedPossibleFilenames
        );
        connect(
            _monitor, &AbstractQueueMonitor::queueResetted,
            this, &QueueEntryInfoFetcher::queueResetted
        );
        connect(
            _monitor, &AbstractQueueMonitor::entriesReceived,
            this, &QueueEntryInfoFetcher::entriesReceived
        );
        connect(
            _monitor, &AbstractQueueMonitor::trackAdded,
            this, &QueueEntryInfoFetcher::trackAdded
        );
        connect(
            _monitor, &AbstractQueueMonitor::trackMoved,
            this, &QueueEntryInfoFetcher::trackMoved
        );

        if (_connection->isConnected()) {
            connected();
        }
    }

    void QueueEntryInfoFetcher::dropInfoFor(quint32 queueId) {
        _infoRequestsSent.remove(queueId);
        _hashRequestsSent.remove(queueId);

        if (!_entries.contains(queueId)) return;

        delete _entries[queueId];
        _entries.remove(queueId);
    }

    QueueEntryInfo* QueueEntryInfoFetcher::entryInfoByQID(quint32 queueID) {
        if (queueID == 0) return 0;

        QueueEntryInfo* info = _entries[queueID];
        if (!info) {
            sendRequest(queueID);
        }
        else if (info->hash().empty() && !_hashRequestsSent.contains(queueID)) {
            sendHashRequest(queueID);
        }

        return info;
    }

    void QueueEntryInfoFetcher::connected() {
        queueResetted(0);
    }

    void QueueEntryInfoFetcher::receivedUserPlayingFor(quint32 userId, QString userLogin)
    {
        Q_UNUSED(userLogin)

        if (_userPlayingFor == userId) return;

        _userPlayingFor = userId;
        emit userPlayingForChanged(userId);
    }

    void QueueEntryInfoFetcher::receivedQueueEntryHash(quint32 queueID,
                                                       QueueEntryType type, FileHash hash)
    {
        qDebug() << "received queue entry hash for QID" << queueID;

        _hashRequestsSent.remove(queueID);

        QueueEntryInfo*& info = _entries[queueID];

        if (!info) {
            info = new QueueEntryInfo(queueID);
        }
        else {
            if (info->type() == type && info->hash() == hash) {
                return; /* no change */
            }
        }

        info->setHash(type, hash);

        enqueueTrackChangeNotification(queueID);
    }

    void QueueEntryInfoFetcher::receivedTrackInfo(quint32 queueID, QueueEntryType type,
                                                  int lengthInSeconds,
                                                  QString title, QString artist)
    {
        qDebug() << "received queue entry info for QID" << queueID;

        _infoRequestsSent.remove(queueID);

        QueueEntryInfo*& info = _entries[queueID];

        if (!info) {
            info = new QueueEntryInfo(queueID);
        }
        else {
            if (info->type() == type && info->lengthInSeconds() == lengthInSeconds
                && info->artist() == artist && info->title() == title)
            {
                return; /* no change */
            }
        }

        info->setInfo(type, lengthInSeconds, title, artist);

        if ((title.trimmed().isEmpty() || artist.trimmed().isEmpty())
            && info->informativeFilename().isEmpty())
        {
            /* no title/artist info available, so we want to display a filename instead */
            _connection->sendPossibleFilenamesRequest(queueID);
        }

        enqueueTrackChangeNotification(queueID);
    }

    void QueueEntryInfoFetcher::receivedPossibleFilenames(quint32 queueID,
                                                          QList<QString> names)
    {
        qDebug() << "received possible filenames for QID" << queueID;

        QueueEntryInfo*& info = _entries[queueID];

        if (!info) {
            info = new QueueEntryInfo(queueID);
        }

        bool changed = info->setPossibleFilenames(names);

        if (!changed) return;

        enqueueTrackChangeNotification(queueID);
    }

    void QueueEntryInfoFetcher::queueResetted(int queueLength) {
        qDebug() << "QueueEntryInfoFetcher::queueResetted called with length"
                 << queueLength;

        _infoRequestsSent.clear();
        _hashRequestsSent.clear();

        qDeleteAll(_entries); /* delete all objects before clearing */
        _entries.clear();
        _entries.reserve(queueLength);

        QList<quint32> IDs;
        IDs.reserve(initialQueueFetchLength);

        //int queueLength = _monitor->queueLength();
        for (int i = 0; i < initialQueueFetchLength; ++i) {
            quint32 qid = _monitor->queueEntry(i);
            if (qid > 0) {
                IDs.append(qid);
                _infoRequestsSent << qid;
                _hashRequestsSent << qid;
            }
        }

        _connection->sendQueueEntryInfoRequest(IDs);
        _connection->sendQueueEntryHashRequest(IDs);
    }

    void QueueEntryInfoFetcher::entriesReceived(int index, QList<quint32> entries) {
        qDebug() << "received QID numbers; index=" << index
                 << "; count=" << entries.size();

        if (index < initialQueueFetchLength) {
            QList<quint32> IDs;
            Q_FOREACH(quint32 entry, entries) {
                if (!_entries.contains(entry)) {
                    IDs.append(entry);
                    _entries[entry] = new QueueEntryInfo(entry);
                }

                _infoRequestsSent << entry;
                _hashRequestsSent << entry;
            }

            qDebug() << "automatically requesting info/hash for" << IDs.size() << "QIDs";
            _connection->sendQueueEntryInfoRequest(IDs);
            _connection->sendQueueEntryHashRequest(IDs);
        }
    }

    void QueueEntryInfoFetcher::trackAdded(int index, quint32 queueID) {
        if (index < initialQueueFetchLength && queueID > 0) {
            /* unlikely, but... */
            if (_entries.contains(queueID)) {
                delete _entries[queueID];
            }

            sendRequest(queueID);
            _entries[queueID] = new QueueEntryInfo(queueID);
        }
    }

    void QueueEntryInfoFetcher::trackMoved(int fromIndex, int toIndex, quint32 queueID) {
        /* was the destination of this move in the tracking zone? */
        if (toIndex < initialQueueFetchLength && queueID > 0
            && !_entries.contains(queueID))
        {
            sendRequest(queueID);
            _entries[queueID] = new QueueEntryInfo(queueID);
        }

        /* check if this moved something OUT of the tracking zone, causing another
           entry to move up INTO the tracking zone */
        if (fromIndex < initialQueueFetchLength && toIndex >= initialQueueFetchLength) {
            int index = initialQueueFetchLength - 1;
            quint32 qid = _monitor->queueEntry(index);
            if (qid > 0 && !_entries.contains(qid)) {
                sendRequest(qid);
                _entries[qid] = new QueueEntryInfo(qid);
            }
        }
    }

    void QueueEntryInfoFetcher::enqueueTrackChangeNotification(quint32 queueID) {
        bool first = _trackChangeNotificationsPending.isEmpty();

        _trackChangeNotificationsPending << queueID;

        if (first) {
            QTimer::singleShot(
                              100, this, &QueueEntryInfoFetcher::emitTracksChangedSignal);
        }
    }

    void QueueEntryInfoFetcher::emitTracksChangedSignal() {
        if (_trackChangeNotificationsPending.isEmpty()) return;

        QList<quint32> list = _trackChangeNotificationsPending.toList();
        _trackChangeNotificationsPending.clear();

        qDebug() << "QueueEntryInfoFetcher: going to emit tracksChanged signal for"
                 << list.size() << "tracks";
        emit tracksChanged(list);
    }

    void QueueEntryInfoFetcher::sendRequest(quint32 queueID) {
        sendHashRequest(queueID);

        if (_infoRequestsSent.contains(queueID))
            return; /* sent already and waiting for an answer */

        _connection->sendQueueEntryInfoRequest(queueID);
        _infoRequestsSent << queueID;
    }

    void QueueEntryInfoFetcher::sendHashRequest(quint32 queueId) {
        if (_hashRequestsSent.contains(queueId)) return;

        QList<uint> ids;
        ids.append(queueId);
        _connection->sendQueueEntryHashRequest(ids);

        _hashRequestsSent << queueId;
    }

}
