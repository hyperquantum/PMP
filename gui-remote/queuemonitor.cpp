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

#include "queuemonitor.h"

#include "common/serverconnection.h"

#include <cstdlib>
#include <QDebug>
#include <QtGlobal>
#include <QTimer>

namespace PMP {

    QueueMonitor::QueueMonitor(QObject* parent, ServerConnection* connection)
     : AbstractQueueMonitor(parent), _connection(connection),
       _waitingForVeryFirstQueueInfo(true),
       _queueLength(0), // _queueLengthSent(0),
       _requestQueueUpTo(5), _queueRequestedUpTo(0)
    {
        connect(
            _connection, &ServerConnection::connected,
            this, &QueueMonitor::connected
        );
        connect(
            _connection, &ServerConnection::receivedQueueContents,
            this, &QueueMonitor::receivedQueueContents
        );
        connect(
            _connection, &ServerConnection::queueEntryRemoved,
            this, &QueueMonitor::queueEntryRemoved
        );
        connect(
            _connection, &ServerConnection::queueEntryAdded,
            this, &QueueMonitor::queueEntryAdded
        );
        connect(
            _connection, &ServerConnection::queueEntryMoved,
            this, &QueueMonitor::queueEntryMoved
        );

        if (_connection->isConnected()) {
            connected();
        }
    }

    void QueueMonitor::connected() {
        _waitingForVeryFirstQueueInfo = true;
        _queueLength = 0;
        //_queueLengthSent = 0;
        _queueRequestedUpTo = 0;
        _queue.clear();

        _queueRequestedUpTo = initialQueueFetchLength;
        _connection->sendQueueFetchRequest(0, initialQueueFetchLength);
    }

    quint32 QueueMonitor::queueEntry(int index) {
        if (index < 0 || index >= _queueLength) return 0;

        /* see if we need to fetch more of the queue */
        if ((index + indexMarginForQueueFetch) >= _requestQueueUpTo) {
            /* we need more of the queue */
            int newFetchUpTo =
                (index + indexMarginForQueueFetch) + 1 + extraRaiseFetchUpTo;

            qDebug() << "QueueMonitor::queueEntry: will raise up-to from"
                << _requestQueueUpTo << "to" << newFetchUpTo;

            _requestQueueUpTo = newFetchUpTo;
            sendNextSlotBatchRequest(queueFetchBatchSize);
        }

        if (index < _queue.size()) {
            return _queue[index];
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
        qDebug() << "QueueMonitor::received queue contents; q-len=" << queueLength << "; startoffset=" << startOffset << "; batch-size=" << queueIDs.size();

        /* is this the first info about the queue we receive? */
        if (_waitingForVeryFirstQueueInfo) {
            _waitingForVeryFirstQueueInfo = false;
            _queueLength = queueLength;
            _queue.clear();
            emit queueResetted(queueLength);
        }

        if (_queueLength != queueLength) {
            qDebug() << " PROBLEM: Q-len inconsistent with what we have (" << _queueLength << "); did we miss queue events?";
            _queueLength = queueLength;
            /* TODO: what now? */
            emit queueResetted(queueLength);
        }

        if (queueIDs.size() == 0) { return; }

        if (startOffset == _queue.size()) {
            qDebug() << " queue contents to be appended to our list";
            _queue.append(queueIDs);
            emit entriesReceived(startOffset, queueIDs);
        }
        else if (startOffset < _queue.size()
                 && startOffset > _queue.size() - queueIDs.size())
        {
            //bool existingChanged = false;
            QList<quint32> changed;
            changed.reserve(queueIDs.size());

            for(int i = 0; i < queueIDs.size(); ++i) {
                int index = startOffset + i;
                if (index < _queue.size()) {
                    if (_queue[index] != queueIDs[i]) {
                        qDebug() << " PROBLEM: unexpected QID change at index" << index
                            <<": old=" << _queue[index] << "; new=" << queueIDs[i];
                        _queue[index] = queueIDs[i];
                    }
                    continue;
                }

                changed.append(queueIDs[i]);
                _queue.append(queueIDs[i]);
            }

            emit entriesReceived(_queue.size() - changed.size(), changed);
        }
        else {
            /* no useful info received */
        }

        /* request the next slice of the queue */
        if (_queueRequestedUpTo > _queueLength) {
            _queueRequestedUpTo = _queueLength;
        }
        else if (_queueRequestedUpTo <= _queue.size()
                 && _queueRequestedUpTo < _queueLength
                 && _queueRequestedUpTo < _requestQueueUpTo)
        {
            qDebug() << " sending next auto queue fetch request -- will request up to" << _requestQueueUpTo;
            sendNextSlotBatchRequest(queueFetchBatchSize);
        }
    }

    void QueueMonitor::queueEntryAdded(quint32 offset, quint32 queueID) {
        int index = (int)offset;

        if (index < 0 || index > _queueLength) {
            /* problem */
            qDebug() << "PROBLEM: QueueMonitor::queueEntryAdded: index out of range: index=" << index << "; Q-len=" << _queueLength;
            return;
        }

        _queueLength++;

        if (index <= _queue.size()) {
            _queue.insert(index, queueID);
        }

        if (index < _queueRequestedUpTo) {
            _queueRequestedUpTo++;
        }

        emit trackAdded((int)offset, queueID);
    }

    void QueueMonitor::queueEntryRemoved(quint32 offset, quint32 queueID) {
        int index = (int)offset;

        if (index < 0 || index >= _queueLength) {
            /* problem */
            qDebug() << "PROBLEM: QueueMonitor::queueEntryRemoved: index out of range: index=" << index << "; Q-len=" << _queueLength;
            return;
        }

        _queueLength--;

        if (index < _queue.size()) {
            if (_queue[index] == queueID || _queue[index] == 0) {
                _queue.removeAt(index);
            }
            else {
                /* TODO: error recovery */
                qDebug() << "PROBLEM: QueueMonitor::queueEntryRemoved: ID does not match; offset=" << offset << "; received ID=" << queueID << "; found ID=" << _queue[offset];
            }
        }

        if (index < _queueRequestedUpTo) {
            _queueRequestedUpTo--;
            sendNextSlotBatchRequest(queueFetchBatchSize);
        }

        emit trackRemoved(index, queueID);
    }

    void QueueMonitor::queueEntryMoved(quint32 fromOffset, quint32 toOffset,
                                       quint32 queueID)
    {
        int fromIndex = (int)fromOffset;
        int toIndex = (int)toOffset;

        if (fromIndex < 0 || fromIndex >= _queueLength) {
            /* problem */
            qDebug() << "PROBLEM: QueueMonitor::queueEntryMoved: fromIndex out of range: fromIndex=" << fromIndex << "; Q-len=" << _queueLength;
            return;
        }
        if (toIndex < 0 || toIndex >= _queueLength) {
            /* problem */
            qDebug() << "PROBLEM: QueueMonitor::queueEntryMoved: toIndex out of range: toIndex=" << toIndex << "; Q-len=" << _queueLength;
            return;
        }

        int oldMyQueueSize = _queue.size();

        if (fromIndex < _queue.size()) {
            if (_queue[fromIndex] == queueID || _queue[fromIndex] == 0) {
                _queue.removeAt(fromIndex);
            }
            else {
                /* TODO: error recovery */
                qDebug() << "PROBLEM: QueueMonitor::queueEntryMoved: ID does not match; fromIndex=" << fromIndex << "; received ID=" << queueID << "; found ID=" << _queue[fromIndex];
            }
        }

        if (toIndex <= _queue.size()) {
            _queue.insert(toIndex, queueID);
        }

        if (oldMyQueueSize > _queue.size()) {
            _queueRequestedUpTo--;
            sendNextSlotBatchRequest(queueFetchBatchSize);
        }
        else if (oldMyQueueSize < _queue.size()) {
            _queueRequestedUpTo++;
        }

        emit trackMoved(fromIndex, toIndex, queueID);
    }

}
