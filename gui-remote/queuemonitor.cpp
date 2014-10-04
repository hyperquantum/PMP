/*
    Copyright (C) 2014, Kevin Andre <hyperquantum@gmail.com>

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

#include "queuemonitor.h"

#include "common/serverconnection.h"

#include <QDebug>
#include <QTimer>

namespace PMP {

    TrackMonitor::TrackMonitor(quint32 queueID/*, int position*/)
     : _queueID(queueID), //_position(position),
       _infoRequestedAlready(queueID == 0), _lengthSeconds(-1)
    {
        //
    }

//    void TrackMonitor::setQueuePosition(int position)
//    {
//        if (position == _position) { return; }
//
//        _position = position;
//    }

    int TrackMonitor::lengthInSeconds() {
        if (!_infoRequestedAlready) {
            _infoRequestedAlready = true;
            emit pleaseAskForMyInfo(_queueID);
        }

        return _lengthSeconds;
    }

    QString TrackMonitor::title() {
        //qDebug() << "TrackMonitor::title() called for queueID " << _queueID;

        if (!_infoRequestedAlready) {
            _infoRequestedAlready = true;
            emit pleaseAskForMyInfo(_queueID);
        }

        return _title;
    }

    QString TrackMonitor::artist() {
        if (!_infoRequestedAlready) {
            _infoRequestedAlready = true;
            emit pleaseAskForMyInfo(_queueID);
        }

        return _artist;
    }

    bool TrackMonitor::setInfo(int lengthInSeconds, QString const& title, QString const& artist) {
        bool changed =
            _lengthSeconds != lengthInSeconds || _title != title || _artist != artist;

        if (!changed) { return false; }

        _lengthSeconds = lengthInSeconds;
        _title = title;
        _artist = artist;

        emit infoChanged();
        return true;
    }

    /* QueueMonitor */

    QueueMonitor::QueueMonitor(QObject* parent, ServerConnection* connection)
     : QObject(parent), _connection(connection),
       _queueLength(0), _queueLengthSent(0),
       _requestQueueUpTo(5), _queueRequestedUpTo(0),
       _trackChangeEventPending(false)
    {
        connect(_connection, SIGNAL(connected()), this, SLOT(connected()));
        connect(_connection, SIGNAL(receivedQueueContents(int, int, QList<quint32>)), this, SLOT(receivedQueueContents(int, int, QList<quint32>)));
        connect(_connection, SIGNAL(queueEntryRemoved(quint32, quint32)), this, SLOT(queueEntryRemoved(quint32, quint32)));
        connect(_connection, SIGNAL(queueEntryAdded(quint32, quint32)), this, SLOT(queueEntryAdded(quint32, quint32)));
        connect(_connection, SIGNAL(receivedTrackInfo(quint32, int, QString, QString)), this, SLOT(receivedTrackInfo(quint32, int, QString, QString)));

        if (_connection->isConnected()) {
            connected();
        }
    }

    void QueueMonitor::connected() {
        _queueLength = 0;
        _queueLengthSent = 0;
        _queueRequestedUpTo = 0;
        _queue.clear();

        _queueRequestedUpTo = 3;
        _connection->sendQueueFetchRequest(0, 3); // only 3 for testing purposes
    }

    quint32 QueueMonitor::queueEntry(int index) {
        if (index < 0) return 0;

        /* see if we need to fetch more of the queue */
        if (index < _queueLength && (index + 3) >= _requestQueueUpTo) {
            qDebug() << "QueueMonitor::queueEntry: will raise up-to, is now" << _requestQueueUpTo;
            _requestQueueUpTo = (index + 3) + 1 + 4; /* we need more of the queue */
            sendNextSlotBatchRequest(4);
        }

        if (index < _queue.size()) {
            quint32 queueID = _queue[index];

            return queueID;
        }

        return 0;
    }

    void QueueMonitor::sendNextSlotBatchRequest(int size) {
        if (size <= 0) { return; }

        int requestCount = size;
        if (_queueLength - requestCount < _queueRequestedUpTo) {
            requestCount = _queueLength - _queueRequestedUpTo;
        }

        if (requestCount <= 0) { return; }

        _connection->sendQueueFetchRequest(_queueRequestedUpTo, requestCount);
        _queueRequestedUpTo += requestCount;
    }

    void QueueMonitor::receivedQueueContents(int queueLength, int startOffset, QList<quint32> queueIDs) {
        qDebug() << "received queue contents; q-length=" << queueLength << "  startoffset=" << startOffset << "  ID count=" << queueIDs.size();

        _queueLength = queueLength;

        /* request the next slice of the queue */
        if (_queueRequestedUpTo > _queueLength) {
            _queueRequestedUpTo = _queueLength;
        }
        else if (_queueRequestedUpTo < _queueLength
            && _queueRequestedUpTo < _requestQueueUpTo)
        {
            qDebug() << " sending next auto queue fetch request -- will request up to" << _requestQueueUpTo;
            sendNextSlotBatchRequest(3);
        }

        if (_queueLengthSent < queueLength) {
            emit tracksInserted(_queueLengthSent, queueLength - 1);
            _queueLengthSent = queueLength;
        }

        if (queueIDs.size() == 0) { return; }

        if (startOffset == _queue.length()) {
            _queue.append(queueIDs);
//            quint32 queueID;
//            int currentPosition = startOffset;
            //foreach(queueID, queueIDs) {
                //trackIsAtPosition(queueID, currentPosition);
                //currentPosition++;
            //}
            return;
        }

//        bool lengthChanged = (_queueLength != queueLength);
//        _queueLength = queueLength;
//
//        int needSize = startOffset + queueIDs.size();
//        if (queueLength < 100 || needSize > queueLength / 2) {
//            needSize = queueLength;
//        }
//
//        if (_queue.length() < needSize) {
//            _queue.reserve(needSize);
//
//            do {
//                _queue.append(0);
//            } while (_queue.length() < needSize);
//        }
//        else if (_queue.length() > queueLength) {
//            do {
//                _queue.removeLast();
//            } while (_queue.length() > queueLength);
//        }
//
//        uint i = startOffset;
//        quint32 QID;
//        foreach(QID, queueIDs) {
//            quint32& entry = _queue[i];
//            if (entry != QID) {
//                entry = QID;
//                /* TODO: raise event */
//            }
//        }
//
//        if (lengthChanged) { emit queueLengthChanged(_queueLength); }
    }

    void QueueMonitor::queueEntryAdded(quint32 offset, quint32 queueID) {
        if ((int)offset > _queueLength) {
            /* problem */
            qDebug() << "PROBLEM: QueueMonitor::queueEntryAdded: Q-length=0, received offset" << offset << "and queueID" << queueID;
            return;
        }

        _queueLength++;

        if ((int)offset <= _queue.size()) {
            _queue.insert(offset, queueID);
        }

        if ((int)offset < _queueRequestedUpTo) {
            _queueRequestedUpTo++;
        }

        if (_queueLengthSent == _queueLength - 1) {
            _queueLengthSent++;
            emit tracksInserted(offset, offset);
        }
        else {
            /* TODO: error correction */
            qDebug() << "PROBLEM: QueueMonitor::queueEntryAdded: _queueLengthSent=" << _queueLengthSent << "; _queueLength=" << _queueLength;
        }
    }

    void QueueMonitor::queueEntryRemoved(quint32 offset, quint32 queueID) {
        if (_queueLength == 0) {
            /* problem */
            qDebug() << "PROBLEM: QueueMonitor::queueEntryRemoved: Q-length=0, received offset" << offset << "and queueID" << queueID;
            return;
        }

        _queueLength--;

        if ((int)offset < _queue.size()) {
            if (_queue[offset] == queueID) {
                _queue.removeAt(offset);
            }
            else {
                /* TODO: error recovery */
                qDebug() << "PROBLEM: QueueMonitor::queueEntryRemoved: ID does not match; offset=" << offset << "; received ID=" << queueID << "; found ID=" << _queue[offset];
            }
        }

        if ((int)offset < _queueRequestedUpTo) {
            _queueRequestedUpTo--;
            sendNextSlotBatchRequest(3);
        }

        if (_queueLengthSent - 1 == _queueLength) {
            _queueLengthSent--;
            emit tracksRemoved(offset, offset);
        }
        else {
            /* TODO: error correction */
            qDebug() << "PROBLEM: QueueMonitor::queueEntryRemoved: _queueLengthSent=" << _queueLengthSent << "; _queueLength=" << _queueLength;
        }
    }

//    void TrackMonitor::trackIsAtPosition(quint32 queueID, int index) {
//        TrackMonitor* t = _tracks[queueID];
//
//        if (t == 0) {
//            t = new TrackMonitor(queueID, index);
//            connect(t, SIGNAL(pleaseAskForMyInfo(quint32)), _connection, SLOT(sendTrackInfoRequest(quint32)));
//            _tracks[queueID] = t;
//        }
//        else {
//            int prevPosition = t->queuePosition();
//            if (prevPosition != index
//                && prevPosition > 0 && prevPosition < _queue.size()
//                && _queue[prevPosition] == queueID)
//            {
//                _queue[prevPosition] = 0; /* something else has taken the place of this track at its old position */
//            }
//
//            t->setQueuePosition(index);
//        }
//    }

    TrackMonitor* QueueMonitor::trackAtPosition(int index) {
        quint32 queueID = queueEntry(index);
        if (queueID == 0) { return 0; }

        return trackFromID(queueID);
    }

    TrackMonitor* QueueMonitor::trackFromID(quint32 queueID) {
        if (queueID == 0) { return 0; }

        TrackMonitor* t = _tracks[queueID];

        if (t == 0) {
            t = new TrackMonitor(queueID);
            connect(t, SIGNAL(pleaseAskForMyInfo(quint32)), _connection, SLOT(sendTrackInfoRequest(quint32)));
            _tracks[queueID] = t;
        }

        return t;
    }

    void QueueMonitor::emitTracksChangedSignal() {
        _trackChangeEventPending = false;

        /* kinda dumb, we signal that all rows have changed */
        if (_queueLengthSent > 0) {
            emit tracksChanged(0, _queueLengthSent - 1);
        }
    }

    void QueueMonitor::receivedTrackInfo(quint32 queueID, int lengthInSeconds, QString title, QString artist) {
        TrackMonitor* t = trackFromID(queueID);

        bool change = false;
        if (t == 0) {
            t = new TrackMonitor(queueID);
            connect(t, SIGNAL(pleaseAskForMyInfo(quint32)), _connection, SLOT(sendTrackInfoRequest(quint32)));
            _tracks[queueID] = t;
            change = true;
        }
        else {
            change = t->setInfo(lengthInSeconds, title, artist);
        }

        if (change && !_trackChangeEventPending) {
            _trackChangeEventPending = true;
            QTimer::singleShot(200, this, SLOT(emitTracksChangedSignal()));
        }
    }

}
