/*
    Copyright (C) 2014-2022, Kevin Andre <hyperquantum@gmail.com>

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

#include "abstractqueuemonitor.h"
#include "queueentryinfostorage.h"
#include "serverconnection.h"

#include <QtDebug>
#include <QTimer>

namespace PMP
{
    namespace
    {
        static const int initialQueueFetchLength = 10;
    }

    QueueEntryInfoFetcher::QueueEntryInfoFetcher(QObject* parent,
                                                 AbstractQueueMonitor* monitor,
                                             QueueEntryInfoStorage* queueEntryInfoStorage,
                                                 ServerConnection* connection)
     : QObject(parent),
       _monitor(monitor),
       _queueEntryInfoStorage(queueEntryInfoStorage),
       _connection(connection)
    {
        connect(
            _connection, &ServerConnection::connected,
            this, &QueueEntryInfoFetcher::connected
        );
        connect(
            _connection, &ServerConnection::disconnected,
            this, &QueueEntryInfoFetcher::connectionBroken
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

        if (_connection->isConnected())
            connected();
    }

    void QueueEntryInfoFetcher::connected()
    {
        queueResetted(0);
    }

    void QueueEntryInfoFetcher::connectionBroken()
    {
        // TODO
    }

    void QueueEntryInfoFetcher::queueResetted(int queueLength)
    {
        qDebug() << "QueueEntryInfoFetcher::queueResetted called with length"
                 << queueLength;

        QList<quint32> queueEntryIds;
        queueEntryIds.reserve(initialQueueFetchLength);

        for (int i = 0; i < initialQueueFetchLength; ++i)
        {
            quint32 qid = _monitor->queueEntry(i);
            if (qid > 0)
                queueEntryIds << qid;
        }

        _queueEntryInfoStorage->refetchEntries(queueEntryIds);
    }

    void QueueEntryInfoFetcher::entriesReceived(int index, QList<quint32> entries)
    {
        qDebug() << "QueueEntryInfoFetcher: received QID numbers; index=" << index
                 << "; count=" << entries.size();

        if (index < initialQueueFetchLength)
        {
            _queueEntryInfoStorage->fetchEntries(entries);
        }
    }

    void QueueEntryInfoFetcher::trackAdded(int index, quint32 queueId)
    {
        if (index < initialQueueFetchLength && queueId > 0)
        {
            _queueEntryInfoStorage->fetchEntry(queueId);
        }
    }

    void QueueEntryInfoFetcher::trackMoved(int fromIndex, int toIndex, quint32 queueId)
    {
        /* was the destination of this move in the tracking zone? */
        if (toIndex < initialQueueFetchLength && queueId > 0)
        {
            _queueEntryInfoStorage->fetchEntry(queueId);
        }

        /* check if this moved something OUT of the tracking zone, causing another
           entry to move up INTO the tracking zone */
        if (fromIndex < initialQueueFetchLength && toIndex >= initialQueueFetchLength)
        {
            int index = initialQueueFetchLength - 1;
            quint32 qid = _monitor->queueEntry(index);

            if (qid > 0)
            {
                _queueEntryInfoStorage->fetchEntry(qid);
            }
        }
    }
}
