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

    class QueueMonitor : public AbstractQueueMonitor
    {
        Q_OBJECT
    public:
        QueueMonitor(ServerConnection* connection);

        void setFetchLimit(int count) override;

        QUuid serverUuid() const override { return _serverUuid; }

        int queueLength() const override { return _queueLength; }
        quint32 queueEntry(int index) override;
        QList<quint32> knownQueuePart() const override { return _queue; }

    private Q_SLOTS:
        void connected();
        void connectionBroken();

        void receivedServerInstanceIdentifier(QUuid uuid);
        void receivedQueueContents(int queueLength, int startOffset,
                                   QList<quint32> queueIDs);
        void queueEntryAdded(qint32 offset, quint32 queueId);
        void queueEntryRemoved(qint32 offset, quint32 queueId);
        void queueEntryMoved(qint32 fromOffset, qint32 toOffset, quint32 queueId);

        void checkIfWeNeedToFetchMore();

        void doReset(int queueLength);

    private:
        void gotRequestForEntryAtIndex(int index);
        void updateQueueLength(int queueLength, bool force);
        bool verifyQueueContentsOldAndNew(int startIndex,
                                          const QList<quint32>& newContent);
        void appendNewQueueContentsAndEmitEntriesReceivedSignal(
                                                        const QList<quint32>& newContent);

        ServerConnection* _connection;
        QUuid _serverUuid;
        int _queueLength;
        int _queueFetchTargetCount;
        int _queueFetchLimit;
        int _queueRequestedEntryCount;
        QList<quint32> _queue; // no need for a QQueue, a QList will suffice
    };
}
#endif
