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

namespace PMP {

    QueueMonitor::QueueMonitor(QObject* parent, ServerConnection* connection)
     : QObject(parent), _connection(connection), _queueLength(0), _queueLengthSent(0)
    {
        connect(_connection, SIGNAL(connected()), this, SLOT(connected()));
        connect(_connection, SIGNAL(receivedQueueContents(int, int, QList<quint32>)), this, SLOT(receivedQueueContents(int, int, QList<quint32>)));
        connect(_connection, SIGNAL(queueEntryRemoved(quint32, quint32)), this, SLOT(queueEntryRemoved(quint32, quint32)));

        if (_connection->isConnected()) {
            connected();
        }
    }

    void QueueMonitor::connected() {
        _queueLength = 0;
        _queue.clear();

        _connection->sendQueueFetchRequest(0, 3); // only 3 for testing purposes
    }

    void QueueMonitor::receivedQueueContents(int queueLength, int startOffset, QList<quint32> queueIDs) {
        qDebug() << "received queue contents; q-length=" << queueLength << "  startoffset=" << startOffset << "  ID count=" << queueIDs.size();

        _queueLength = queueLength;

        if (_queueLengthSent < queueLength) {
            emit tracksInserted(_queueLengthSent, queueLength - 1);
            _queueLengthSent = queueLength;
        }

        if (queueIDs.size() == 0) { return; }

        if (startOffset == _queue.length()) {
            _queue.append(queueIDs);
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

    void QueueMonitor::queueEntryRemoved(quint32 offset, quint32 queueID) {
        if (_queueLength == 0) {
            /* problem */
            return;
        }

        _queueLength--;

        if ((int)offset < _queue.size()) {
            _queue.removeAt(offset);
        }

        if (_queueLengthSent > 0) {
            _queueLengthSent--;
            emit tracksRemoved(offset, offset);
        }


//        if (_queueLength > 0) {
//            _queueLength--;
//        }
//        else {
//            /* problem... */
//            /* TODO */
//        }
//
//        if ((int)offset < _queue.size()) {
//            quint32 entry = _queue[offset];
//
//            if (entry == 0 || entry == queueID) {
//                /* OK */
//                _queue.removeAt(offset);
//                /* TODO: raise event */
//            }
//            else {
//                /* problem... */
//                /* TODO */
//            }
//        }
//
//        emit queueLengthChanged(_queueLength);
    }

}
