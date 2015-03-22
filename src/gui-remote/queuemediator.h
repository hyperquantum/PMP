/*
    Copyright (C) 2015, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_QUEUEMEDIATOR_H
#define PMP_QUEUEMEDIATOR_H

#include "abstractqueuemonitor.h"

namespace PMP {

    class QueueMonitor;
    class ServerConnection;

    class QueueMediator : public AbstractQueueMonitor {
        Q_OBJECT
    public:
        QueueMediator(QObject* parent, QueueMonitor* monitor,
                      ServerConnection* connection);

        QUuid serverUuid() const;

        int queueLength() const { return _queueLength; }
        quint32 queueEntry(int index);
        QList<quint32> knownQueuePart() const { return _myQueue; }

        void removeTrack(int index, quint32 queueID);
        void moveTrack(int fromIndex, int toIndex, quint32 queueID);

    private slots:
        void resetQueue(int queueLength);
        void entriesReceivedAtServer(int index, QList<quint32> entries);
        void trackAddedAtServer(int index, quint32 queueID);
        void trackRemovedAtServer(int index, quint32 queueID);
        void trackMovedAtServer(int fromIndex, int toIndex, quint32 queueID);

    private:
        class Operation;
        class InfoOperation;
        class DeleteOperation;
        class AddOperation;
        class MoveOperation;

        bool doLocalOperation(Operation* op);
        bool handleServerOperation(Operation* op);

        QueueMonitor* _sourceMonitor;
        ServerConnection* _connection;
        int _queueLength;
        QList<quint32> _myQueue;
        QList<Operation*> _pendingOperations;
    };
}
#endif
