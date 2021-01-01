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

#ifndef PMP_QUEUEMONITOR_H
#define PMP_QUEUEMONITOR_H

#include "abstractqueuemonitor.h"

namespace PMP {

    class ServerConnection;

    class QueueMonitor : public AbstractQueueMonitor {
        Q_OBJECT

    public:
        QueueMonitor(QObject* parent, ServerConnection* connection);

        QUuid serverUuid() const { return _serverUuid; }

        int queueLength() const { return _queueLength; }
        quint32 queueEntry(int index);
        QList<quint32> knownQueuePart() const { return _queue; }

    private Q_SLOTS:
        void connected();
        void receivedServerInstanceIdentifier(QUuid uuid);
        void receivedQueueContents(int queueLength, int startOffset,
                                   QList<quint32> queueIDs);
        void queueEntryAdded(qint32 offset, quint32 queueID);
        void queueEntryRemoved(qint32 offset, quint32 queueID);
        void queueEntryMoved(qint32 fromOffset, qint32 toOffset, quint32 queueID);

        void sendNextSlotBatchRequest(int size);

        void doReset(int queueLength);

    private:
        static const int initialQueueFetchLength = 10;
        static const int indexMarginForQueueFetch = 3;
        static const int extraRaiseFetchUpTo = 4;
        static const int queueFetchBatchSize = 4;

        ServerConnection* _connection;
        QUuid _serverUuid;
        bool _waitingForVeryFirstQueueInfo;
        int _queueLength;
        int _requestQueueUpTo;
        int _queueRequestedUpTo;
        QList<quint32> _queue; // no need for a QQueue, a QList will suffice
    };
}
#endif
