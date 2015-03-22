/*
    Copyright (C) 2014-2015, Kevin Andre <hyperquantum@gmail.com>

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
     : _queueID(queueID), _lengthSeconds(-1)
    {
        //
    }

    void QueueEntryInfo::setInfo(int lengthInSeconds, QString const& title,
                                 QString const& artist)
    {
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
     : QObject(parent), _monitor(monitor), _connection(connection)
    {
        connect(
            _connection, &ServerConnection::connected,
            this, &QueueEntryInfoFetcher::connected
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
            _monitor, &AbstractQueueMonitor::trackRemoved,
            this, &QueueEntryInfoFetcher::trackRemoved
        );
        connect(
            _monitor, &AbstractQueueMonitor::trackMoved,
            this, &QueueEntryInfoFetcher::trackMoved
        );

        if (_connection->isConnected()) {
            connected();
        }
    }

    QueueEntryInfo* QueueEntryInfoFetcher::entryInfoByQID(quint32 queueID) {
        if (queueID == 0) return 0;

        QueueEntryInfo* info = _entries[queueID];
        if (info == 0) {
            _connection->sendTrackInfoRequest(queueID);
        }

        return info;
    }

//    QueueEntryInfo* QueueEntryInfoFetcher::entryInfoByIndex(int index) {
//        if (index < 0 || index >= _monitor->queueLength()) {
//            return 0;
//        }
//
//        quint32 qid = _monitor->queueEntry(index);
//        if (qid == 0) {
//            return 0;
//        }
//
//        return entryInfoByQID(qid);
//    }

    void QueueEntryInfoFetcher::connected() {
        queueResetted(0);
    }

    void QueueEntryInfoFetcher::receivedTrackInfo(quint32 queueID, int lengthInSeconds,
                                                  QString title, QString artist)
    {
        QueueEntryInfo*& info = _entries[queueID];

        if (info == 0) {
            info = new QueueEntryInfo(queueID);
        }
        else {
            if (info->artist() == artist && info->title() == title
                && info->lengthInSeconds() == lengthInSeconds)
            {
                return; /* no change */
            }
        }

        info->setInfo(lengthInSeconds, title, artist);

        if ((title.trimmed().isEmpty() || artist.trimmed().isEmpty())
            && info->informativeFilename().isEmpty())
        {
            _connection->sendPossibleFilenamesRequest(queueID);
        }

        trackChangeToSchedule(queueID);
    }

    void QueueEntryInfoFetcher::receivedPossibleFilenames(quint32 queueID,
                                                          QList<QString> names)
    {
        QueueEntryInfo*& info = _entries[queueID];

        if (info == 0) {
            info = new QueueEntryInfo(queueID);
        }

        bool changed = info->setPossibleFilenames(names);

        if (!changed) return;

        trackChangeToSchedule(queueID);
    }

    void QueueEntryInfoFetcher::queueResetted(int queueLength) {
        qDebug() << "QueueEntryInfoFetcher::queueResetted called with length" << queueLength;

        _entries.clear(); // FIXME: memory leak
        _entries.reserve(queueLength);

        QList<quint32> IDs;
        IDs.reserve(initialQueueFetchLength);

        //int queueLength = _monitor->queueLength();
        for (int i = 0; i < initialQueueFetchLength; ++i) {
            quint32 qid = _monitor->queueEntry(i);
            if (qid > 0) {
                IDs.append(qid);
            }
        }

        _connection->sendTrackInfoRequest(IDs);
    }

    void QueueEntryInfoFetcher::entriesReceived(int index, QList<quint32> entries) {
        if (index < initialQueueFetchLength) {
            QList<quint32> IDs;
            Q_FOREACH(quint32 entry, entries) {
                if (!_entries.contains(entry)) {
                    IDs.append(entry);
                    _entries[entry] = new QueueEntryInfo(entry);
                }
            }

            _connection->sendTrackInfoRequest(IDs);
        }
    }

    void QueueEntryInfoFetcher::trackAdded(int index, quint32 queueID) {
        if (index < initialQueueFetchLength && queueID > 0) {
            _connection->sendTrackInfoRequest(queueID);
            _entries[queueID] = new QueueEntryInfo(queueID);
        }
    }

    void QueueEntryInfoFetcher::trackRemoved(int index, quint32 queueID) {
        if (!_entries.contains(queueID)) return;

        delete _entries[queueID];
        _entries.remove(queueID);
    }

    void QueueEntryInfoFetcher::trackMoved(int fromIndex, int toIndex, quint32 queueID) {
        /* was the destination of this move in the tracking zone? */
        if (toIndex < initialQueueFetchLength && queueID > 0
            && !_entries.contains(queueID))
        {
            _connection->sendTrackInfoRequest(queueID);
            _entries[queueID] = new QueueEntryInfo(queueID);
        }

        /* check if this moved something OUT of the tracking zone, causing another
           entry to move up INTO the tracking zone */
        if (fromIndex < initialQueueFetchLength && toIndex >= initialQueueFetchLength) {
            int index = initialQueueFetchLength - 1;
            quint32 qid = _monitor->queueEntry(index);
            if (qid > 0 && !_entries.contains(qid)) {
                _connection->sendTrackInfoRequest(qid);
                _entries[qid] = new QueueEntryInfo(qid);
            }
        }
    }

    void QueueEntryInfoFetcher::trackChangeToSchedule(quint32 queueID) {
        bool first = _trackChangeNotificationsPending.isEmpty();

        _trackChangeNotificationsPending << queueID;

        if (first) {
            QTimer::singleShot(100, this, SLOT(emitTracksChangedSignal()));
        }
    }

    void QueueEntryInfoFetcher::emitTracksChangedSignal() {
        if (_trackChangeNotificationsPending.isEmpty()) return;

        QList<quint32> list = _trackChangeNotificationsPending.toList();
        _trackChangeNotificationsPending.clear();

        qDebug() << "QueueEntryInfoFetcher: going to emit tracksChanged signal for" << list.size() << "tracks";
        emit tracksChanged(list);
    }

}