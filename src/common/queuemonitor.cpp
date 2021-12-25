/*
    Copyright (C) 2014-2021, Kevin Andre <hyperquantum@gmail.com>

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

#include "serverconnection.h"

#include <QDebug>
#include <QtGlobal>
#include <QTimer>

namespace PMP
{
    namespace
    {
        static const int initialQueueFetchLength = 10;
        static const int indexMarginForQueueFetch = 5;
        static const int extraRaiseFetchUpTo = 20;
        static const int queueFetchBatchSize = 10;
    }

    QueueMonitor::QueueMonitor(ServerConnection* connection)
     : AbstractQueueMonitor(connection),
       _connection(connection),
       _queueLength(0),
       _queueFetchTargetCount(initialQueueFetchLength),
       _queueFetchLimit(-1),
       _queueRequestedEntryCount(0),
       _fetchCompletedEmitted(false)
    {
        connect(
            _connection, &ServerConnection::connected,
            this, &QueueMonitor::connected
        );
        connect(
            _connection, &ServerConnection::disconnected,
            this, &QueueMonitor::connectionBroken
        );
        connect(
            _connection, &ServerConnection::receivedServerInstanceIdentifier,
            this, &QueueMonitor::receivedServerInstanceIdentifier
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

        if (_connection->isConnected())
            connected();
    }

    void QueueMonitor::setFetchLimit(int count)
    {
        qDebug() << "QueueMonitor: fetch limit set to" << count;
        _queueFetchLimit = count;

        if (_queueFetchTargetCount > _queueFetchLimit)
            _queueFetchTargetCount = _queueFetchLimit;
    }

    void QueueMonitor::connected()
    {
        _connection->sendServerInstanceIdentifierRequest();

        sendInitialQueueFetchRequest();
    }

    void QueueMonitor::connectionBroken()
    {
        updateQueueLength(0, false);
    }

    void QueueMonitor::doReset(int queueLength)
    {
        qDebug() << "QueueMonitor: resetting queue to length" << queueLength;

        _fetchCompletedEmitted = false;
        updateQueueLength(queueLength, true);
    }

    void QueueMonitor::receivedServerInstanceIdentifier(QUuid uuid)
    {
        _serverUuid = uuid;
    }

    quint32 QueueMonitor::queueEntry(int index)
    {
        if (index < 0 || index >= _queueLength)
            return 0; /* invalid index */

        gotRequestForEntryAtIndex(index);

        if (index < _queue.size())
            return _queue[index];

        return 0;
    }

    void QueueMonitor::receivedQueueContents(int queueLength, int startOffset,
                                             QList<quint32> queueIDs)
    {
        qDebug() << "QueueMonitor: received queue contents:"
                 << "queue length:" << queueLength
                 << "; offset:" << startOffset
                 << "; batch-size:" << queueIDs.size();

        /* See if queue length has changed; this happens at first load or when
         * inconsistencies are discovered. */
        updateQueueLength(queueLength, false);

        if (queueIDs.size() == 0)
        {
            checkFetchCompletedState();
            return;
        }

        if (startOffset == _queue.size())
        {
            qDebug() << "QueueMonitor: appending queue contents to our list";
            appendNewQueueContentsAndEmitEntriesReceivedSignal(queueIDs);
        }
        else if (startOffset > _queue.size())
        {
            qDebug() << "QueueMonitor: queue contents is beyond our list";

            /* will need to fetch again */
            _queueRequestedEntryCount = _queue.size();
        }
        else /* startOffset < _queue.size() */
        {
            qDebug() << "QueueMonitor: doing merge of queue contents";

            if (!verifyQueueContentsOldAndNew(startOffset, queueIDs))
            {
                doReset(queueLength);
                return;
            }

            /* append the rest */
            int newContentIndex = _queue.size() - startOffset;
            if (newContentIndex < queueIDs.size())
            {
                auto toAppend = queueIDs.mid(newContentIndex);
                appendNewQueueContentsAndEmitEntriesReceivedSignal(toAppend);
            }
        }

        checkIfWeNeedToFetchMore();
    }

    void QueueMonitor::queueEntryAdded(qint32 offset, quint32 queueId)
    {
        int index = (int)offset;
        qDebug() << "QueueMonitor: QID" << queueId << "was added at index" << index;

        if (index < 0 || index > _queueLength)
        {
            /* problem */
            qWarning() << "QueueMonitor: queueEntryAdded: index out of range: index="
                       << index << "; Q-len=" << _queueLength;

            if (index > 0)
            {
                /* find out what's going on, this will trigger a reset */
                _connection->sendQueueFetchRequest(_queueLength, 1);
            }
            return;
        }

        _queueLength++;

        if (index <= _queue.size())
            _queue.insert(index, queueId);

        if (index < _queueRequestedEntryCount)
            _queueRequestedEntryCount++;

        Q_EMIT trackAdded((int)offset, queueId);
    }

    void QueueMonitor::queueEntryRemoved(qint32 offset, quint32 queueId)
    {
        int index = (int)offset;
        qDebug() << "QueueMonitor: QID" << queueId << "was removed at index" << index;

        if (index < 0 || index >= _queueLength)
        {
            /* problem */
            qWarning() << "QueueMonitor: queueEntryRemoved: index out of range: index="
                       << index << "; Q-len=" << _queueLength;

            if (index > 0)
            {
                /* find out what's going on, this will trigger a reset */
                _connection->sendQueueFetchRequest(_queueLength, 1);
            }
            return;
        }

        _queueLength--;

        if (index < _queue.size())
        {
            if (_queue[index] == queueId || _queue[index] == 0)
            {
                _queue.removeAt(index);
            }
            else
            {
                /* TODO: error recovery */
                qWarning() << "QueueMonitor: queueEntryRemoved: ID does not match;"
                           << "offset=" << offset << "; received ID=" << queueId
                           << "; found ID=" << _queue[offset];

                /* find out what's going on, this will trigger a reset */
                _connection->sendQueueFetchRequest(index, 1);
                return;
            }
        }

        if (index < _queueRequestedEntryCount)
        {
            _queueRequestedEntryCount--;
            checkIfWeNeedToFetchMore();
        }

        Q_EMIT trackRemoved(index, queueId);
    }

    void QueueMonitor::queueEntryMoved(qint32 fromOffset, qint32 toOffset,
                                       quint32 queueId)
    {
        int fromIndex = (int)fromOffset;
        int toIndex = (int)toOffset;
        qDebug() << "QueueMonitor: QID" << queueId << "was moved from index" << fromIndex
                 << "to index" << toIndex;

        if (fromIndex < 0 || fromIndex >= _queueLength)
        {
            /* problem */
            qWarning() << "QueueMonitor: queueEntryMoved: fromIndex out of range:"
                       << "fromIndex=" << fromIndex << "; Q-len=" << _queueLength;

            if (fromIndex > 0)
            {
                /* find out what's going on, this will trigger a reset */
                _connection->sendQueueFetchRequest(_queueLength, 1);
            }
            return;
        }
        if (toIndex < 0 || toIndex >= _queueLength)
        {
            /* problem */
            qWarning() << "QueueMonitor: queueEntryMoved: toIndex out of range:"
                       << "toIndex=" << toIndex << "; Q-len=" << _queueLength;

            if (toIndex > 0)
            {
                /* find out what's going on, this will trigger a reset */
                _connection->sendQueueFetchRequest(_queueLength, 1);
            }
            return;
        }

        int oldMyQueueSize = _queue.size();

        if (fromIndex < _queue.size())
        {
            if (_queue[fromIndex] == queueId || _queue[fromIndex] == 0)
            {
                _queue.removeAt(fromIndex);
            }
            else
            {
                qWarning() << "QueueMonitor: queueEntryMoved: ID does not match;"
                           << "fromIndex=" << fromIndex << "; received ID=" << queueId
                           << "; found ID=" << _queue[fromIndex];

                /* find out what's going on, this will trigger a reset */
                _connection->sendQueueFetchRequest(fromIndex, 1);
                return;
            }
        }

        if (toIndex <= _queue.size())
            _queue.insert(toIndex, queueId);

        if (oldMyQueueSize > _queue.size())
        {
            _queueRequestedEntryCount--;
            checkIfWeNeedToFetchMore();
        }
        else if (oldMyQueueSize < _queue.size())
        {
            _queueRequestedEntryCount++;
        }

        Q_EMIT trackMoved(fromIndex, toIndex, queueId);
    }

    void QueueMonitor::checkIfWeNeedToFetchMore()
    {
        checkFetchCompletedState();

        /* no need to fetch any more if we got the entire queue already */
        if (_queueRequestedEntryCount >= _queueLength)
        {
            _queueRequestedEntryCount = _queueLength;
            return;
        }

        /* we can stop fetching as soon as we have reached the target count */
        if (_queueRequestedEntryCount >= _queueFetchTargetCount)
            return;

        /* wait until all previous fetch requests have been answered */
        if (_queueRequestedEntryCount > _queue.size())
            return;

        qDebug() << "QueueMonitor: sending queue fetch request:"
                 << " index:" << _queueRequestedEntryCount
                 << "; count:" << queueFetchBatchSize;

        _connection->sendQueueFetchRequest(_queueRequestedEntryCount, queueFetchBatchSize);
        _queueRequestedEntryCount += queueFetchBatchSize;
    }

    void QueueMonitor::gotRequestForEntryAtIndex(int index)
    {
        /* do we need to raise the queue fetch target? */

        if (index < _queueFetchTargetCount - indexMarginForQueueFetch)
            return; /* no, not yet */

        if (_queueFetchLimit >= 0 && _queueFetchTargetCount >= _queueFetchLimit)
            return; /* no, we have reached the fetch limit */

        int newFetchCount = _queueFetchTargetCount + extraRaiseFetchUpTo;

        /* don't cross the fetch limit */
        if (_queueFetchLimit >= 0 && newFetchCount > _queueFetchLimit)
            newFetchCount = _queueFetchLimit;

        qDebug() << "QueueMonitor: will raise fetch target count from"
            << _queueFetchTargetCount << "to" << newFetchCount
            << "because index" << index << "was requested";

        _queueFetchTargetCount = newFetchCount;
        checkIfWeNeedToFetchMore();
    }

    void QueueMonitor::updateQueueLength(int queueLength, bool forceReload)
    {
        if (queueLength == _queueLength && !forceReload)
            return; /* no change */

        qDebug() << "QueueMonitor: queue length changing from" << _queueLength
                 << "to" << queueLength;

        _queueLength = queueLength;

        if (forceReload || _queue.size() > 0)
        {
            qDebug() << "QueueMonitor: going to reload the queue";
            _queueRequestedEntryCount = 0;
            _queue.clear();
            sendInitialQueueFetchRequest();
        }

        Q_EMIT queueResetted(queueLength);
    }

    void QueueMonitor::sendInitialQueueFetchRequest()
    {
        _queueRequestedEntryCount = initialQueueFetchLength;
        _connection->sendQueueFetchRequest(0, initialQueueFetchLength);
    }

    bool QueueMonitor::verifyQueueContentsOldAndNew(int startIndex,
                                                    const QList<quint32>& newContent)
    {
        for(int newContentIndex = 0;
            newContentIndex < newContent.size();
            ++newContentIndex)
        {
            int queueIndex = startIndex + newContentIndex;
            if (queueIndex >= _queue.size())
                return true;

            if (_queue[queueIndex] == newContent[newContentIndex])
                continue;

            qWarning() << "QueueMonitor: unexpected QID change at index" << queueIndex
                       << ": old=" << _queue[queueIndex]
                       << "; new=" << newContent[newContentIndex];

            return false;
        }

        return true;
    }

    void QueueMonitor::appendNewQueueContentsAndEmitEntriesReceivedSignal(
                                                         const QList<quint32>& newContent)
    {
        auto previousQueueSize = _queue.size();
        _queue.append(newContent);
        Q_EMIT entriesReceived(previousQueueSize, newContent);
    }

    void QueueMonitor::checkFetchCompletedState()
    {
        if (_fetchCompletedEmitted)
            return;

        if (_queue.size() == _queueLength
                || _queue.size() >= _queueFetchLimit)
        {
            _fetchCompletedEmitted = true;

            qDebug() << "QueueMonitor: going to emit fetchCompleted signal";
            Q_EMIT fetchCompleted();
        }
    }

}
