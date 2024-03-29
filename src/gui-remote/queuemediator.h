/*
    Copyright (C) 2015-2023, Kevin Andre <hyperquantum@gmail.com>

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

#include "client/abstractqueuemonitor.h"
#include "client/localhashid.h"

namespace PMP::Client
{
    class AbstractQueueMonitor;
    class QueueController;
    class ServerInterface;
}

namespace PMP
{
    class QueueMediator : public Client::AbstractQueueMonitor
    {
        Q_OBJECT
    public:
        QueueMediator(QObject* parent, AbstractQueueMonitor* monitor,
                      Client::ServerInterface* serverInterface);

        void setFetchLimit(int count) override;

        QUuid serverUuid() const override;

        bool isQueueLengthKnown() const override
        {
            return _sourceMonitor->isQueueLengthKnown();
        }

        int queueLength() const override { return _queueLength; }
        quint32 queueEntry(int index) override;
        QList<quint32> knownQueuePart() const override { return _myQueue; }
        bool isFetchCompleted() const override;

        void removeTrack(int index, quint32 queueID);
        void moveTrack(int fromIndex, int toIndex, quint32 queueID);
        void moveTrackToEnd(int fromIndex, quint32 queueId);

        void insertFileAsync(int index, Client::LocalHashId hashId);
        void duplicateEntryAsync(quint32 queueID);
        bool canDuplicateEntry(quint32 queueID) const;

    private Q_SLOTS:
        void resetQueue(int queueLength);
        void entriesReceivedAtServer(int index, QList<quint32> entries);
        void trackAddedAtServer(int index, quint32 queueID);
        void trackRemovedAtServer(int index, quint32 queueID);
        void trackMovedAtServer(int fromIndex, int toIndex, quint32 queueID);

    private:
        Client::QueueController& queueController() const;

        class Operation;
        class InfoOperation;
        class DeleteOperation;
        class AddOperation;
        class MoveOperation;

        void doResetQueue();
        bool doLocalOperation(Operation* op);
        bool handleServerOperation(Operation* op);

        AbstractQueueMonitor* _sourceMonitor;
        Client::ServerInterface* _serverInterface;
        int _queueLength;
        QList<quint32> _myQueue;
        QList<Operation*> _pendingOperations;
    };
}
#endif
