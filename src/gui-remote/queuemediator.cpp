/*
    Copyright (C) 2015-2021, Kevin Andre <hyperquantum@gmail.com>

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

#include "queuemediator.h"

#include "common/clientserverinterface.h"
#include "common/queuecontroller.h"
#include "common/queuemonitor.h"
#include "common/serverconnection.h"

#include <QtDebug>

namespace PMP {

    /* ========================== Operation ========================== */

    class QueueMediator::Operation {
    public:
        virtual ~Operation() {}

        virtual bool execute(QueueMediator& mediator, bool sendToServer) = 0;
        virtual bool rollback(QueueMediator& mediator) = 0;

        virtual bool equals(Operation* op) { Q_UNUSED(op) return false; }
        virtual bool equals(InfoOperation* op) { Q_UNUSED(op) return false; }
        virtual bool equals(DeleteOperation* op) { Q_UNUSED(op) return false; }
        virtual bool equals(AddOperation* op) { Q_UNUSED(op) return false; }
        virtual bool equals(MoveOperation* op) { Q_UNUSED(op) return false; }

    };

    /* ========================== DeleteOperation ========================== */

    class QueueMediator::DeleteOperation : public QueueMediator::Operation {
    public:
        DeleteOperation(int index, quint32 queueID)
         : _index(index), _queueID(queueID)
        {
            //
        }

        virtual ~DeleteOperation() {}

        virtual bool execute(QueueMediator& mediator, bool sendToServer);
        virtual bool rollback(QueueMediator& mediator);

        virtual bool equals(Operation* op) { return op != 0 && op->equals(this); }

        virtual bool equals(DeleteOperation* op) {
            return op != 0 && _index == op->_index && _queueID == op->_queueID;
        }

    private:
        int _index;
        quint32 _queueID;
    };

    bool QueueMediator::DeleteOperation::execute(QueueMediator& mediator,
                                                 bool sendToServer)
    {
        if (_index < 0 || _index >= mediator._queueLength
            || (_index < mediator._myQueue.size()
                && mediator._myQueue[_index] != _queueID))
        {
            return false; /* PROBLEM or INCONSISTENCY */
        }

        if (sendToServer) {
            mediator.queueController().deleteQueueEntry(_queueID);
        }

        if (_index < mediator._myQueue.size()) {
            mediator._myQueue.removeAt(_index);
        }

        mediator._queueLength--;

        Q_EMIT mediator.trackRemoved(_index, _queueID);
        return true;
    }

    bool QueueMediator::DeleteOperation::rollback(QueueMediator& mediator) {
        if (_index < 0 || _index > mediator._myQueue.size()) {
            /* how could this have ever been applied? */
            return false;
        }

        mediator._myQueue.insert(_index, _queueID);
        mediator._queueLength++;

        Q_EMIT mediator.trackAdded(_index, _queueID);
        return true;
    }

    /* ========================== AddOperation ========================== */

    class QueueMediator::AddOperation : public QueueMediator::Operation {
    public:
        AddOperation(int index, quint32 queueID)
         : _index(index), _queueID(queueID)
        {
            //
        }

        virtual ~AddOperation() {}

        virtual bool execute(QueueMediator& mediator, bool sendToServer);
        virtual bool rollback(QueueMediator& mediator);

        virtual bool equals(Operation* op) { return op != 0 && op->equals(this); }

        virtual bool equals(AddOperation* op) {
            return op != 0 && _index == op->_index && _queueID == op->_queueID;
        }

    private:
        int _index;
        quint32 _queueID;
    };

    bool QueueMediator::AddOperation::execute(QueueMediator& mediator,
                                              bool sendToServer)
    {
        if (_index < 0 || _index > mediator._queueLength)
        {
            return false; /* PROBLEM or INCONSISTENCY */
        }

        if (sendToServer) {
            /* NOT POSSIBLE, because the server determines the queue ID */
            //mediator._connection->addQueueEntry(_queueID);
            return false;
        }

        if (_index <= mediator._myQueue.size()) {
            mediator._myQueue.insert(_index, _queueID);
        }

        mediator._queueLength++;

        Q_EMIT mediator.trackAdded(_index, _queueID);
        return true;
    }

    bool QueueMediator::AddOperation::rollback(QueueMediator& mediator) {
        Q_UNUSED(mediator)

        /* Rollback support for AddOperation is not needed, as this is
           only a server-side operation. */
        return false;
    }

    /* ========================== MoveOperation ========================== */

    class QueueMediator::MoveOperation : public QueueMediator::Operation {
    public:
        MoveOperation(int fromIndex, int toIndex, quint32 queueID)
         : _fromIndex(fromIndex), _toIndex(toIndex), _queueID(queueID)
        {
            //
        }

        virtual ~MoveOperation() {}

        virtual bool execute(QueueMediator& mediator, bool sendToServer);
        virtual bool rollback(QueueMediator& mediator);

        virtual bool equals(Operation* op) { return op != 0 && op->equals(this); }

        virtual bool equals(MoveOperation* op) {
            return op != 0
                && _fromIndex == op->_fromIndex
                && _toIndex == op->_toIndex
                && _queueID == op->_queueID;
        }

    private:
        int _fromIndex;
        int _toIndex;
        quint32 _queueID;
    };

    bool QueueMediator::MoveOperation::execute(QueueMediator& mediator,
                                               bool sendToServer)
    {
        if (_fromIndex < 0 || _toIndex < 0
            || _fromIndex >= mediator._queueLength
            || _toIndex >= mediator._queueLength
            || (_fromIndex < mediator._myQueue.size()
                && mediator._myQueue[_fromIndex] != _queueID))
        {
            return false; /* PROBLEM or INCONSISTENCY */
        }

        if (sendToServer) {
            mediator.queueController().moveQueueEntry(_queueID, _toIndex - _fromIndex);
        }

        if (_fromIndex < mediator._myQueue.size()) {
            mediator._myQueue.removeAt(_fromIndex);
        }

        if (_toIndex <= mediator._myQueue.size()) {
            mediator._myQueue.insert(_toIndex, _queueID);
        }

        Q_EMIT mediator.trackMoved(_fromIndex, _toIndex, _queueID);
        return true;
    }

    bool QueueMediator::MoveOperation::rollback(QueueMediator& mediator) {
        if (_fromIndex < 0 || _toIndex < 0
            || _fromIndex >= mediator._queueLength
            || _toIndex >= mediator._queueLength
            || (_toIndex < mediator._myQueue.size()
                && mediator._myQueue[_toIndex] != _queueID))
        {
            /* how could this have ever been applied? */
            return false;
        }

        if (_toIndex < mediator._myQueue.size()) {
            mediator._myQueue.removeAt(_toIndex);
        }

        if (_fromIndex <= mediator._myQueue.size()) {
            mediator._myQueue.insert(_fromIndex, _queueID);
        }

        Q_EMIT mediator.trackMoved(_toIndex, _fromIndex, _queueID);
        return true;
    }

    /* ========================== InfoOperation ========================== */

    class QueueMediator::InfoOperation : public QueueMediator::Operation {
    public:
        InfoOperation(int index, QList<quint32> entries)
         : _index(index), _entries(entries)
        {
            //
        }

        virtual ~InfoOperation() {}

        virtual bool execute(QueueMediator& mediator, bool sendToServer);
        virtual bool rollback(QueueMediator& mediator);

        virtual bool equals(Operation* op) { return op != 0 && op->equals(this); }

        virtual bool equals(InfoOperation* op) {
            if (op == 0 || _index != op->_index)
            {
                return false;
            }

            return _entries == op->_entries;
        }

    private:
        int _index;
        QList<quint32> _entries;
    };

    bool QueueMediator::InfoOperation::execute(QueueMediator& mediator,
                                               bool sendToServer)
    {
        if (_index < 0 || _entries.size() > mediator._queueLength
            || _index > mediator._queueLength - _entries.size())
        {
            qDebug() << "QueueMediator::InfoOperation::execute: INCONSISTENCY detected";
            return false; /* PROBLEM or INCONSISTENCY */
        }

        if (sendToServer) {
            /* NOT POSSIBLE, because this operation is never requested by the client */
        }

        if (mediator._myQueue.size() < _index + _entries.size()) {
            qDebug() << " need to expand mediator list";
            mediator._myQueue.reserve(_index + _entries.size());
            do {
                mediator._myQueue.append(0);
            } while (mediator._myQueue.size() < _index + _entries.size());
        }

        bool change = false;
        for (int i = 0; i < _entries.size(); ++i) {
            int queueIndex = _index + i;

            quint32 existing = mediator._myQueue[queueIndex];
            if (existing == _entries[i]) continue;

            if (existing != 0) {
                qDebug() << "QueueMediator::InfoOperation::execute: INCONSISTENCY detected (2)";
                return false; /* INCONSISTENCY detected */
            }

            mediator._myQueue[queueIndex] = _entries[i];
            change = true;
        }

        if (change) {
            Q_EMIT mediator.entriesReceived(_index, _entries);
        }

        return true;
    }

    bool QueueMediator::InfoOperation::rollback(QueueMediator& mediator) {
        Q_UNUSED(mediator)

        /* NOT NECESSARY */
        return false;
    }

    /* ========================== QueueMediator ========================== */

    QueueMediator::QueueMediator(QObject* parent, AbstractQueueMonitor* monitor,
                                 ClientServerInterface* clientServerInterface)
     : AbstractQueueMonitor(parent),
       _sourceMonitor(monitor),
       _clientServerInterface(clientServerInterface)
    {
        _myQueue = monitor->knownQueuePart();
        _queueLength = monitor->queueLength();

        connect(
            monitor, &AbstractQueueMonitor::fetchCompleted,
            this, &QueueMediator::fetchCompleted
        );
        connect(
            monitor, &AbstractQueueMonitor::queueResetted,
            this, &QueueMediator::resetQueue
        );
        connect(
            monitor, &AbstractQueueMonitor::entriesReceived,
            this, &QueueMediator::entriesReceivedAtServer
        );
        connect(
            monitor, &AbstractQueueMonitor::trackAdded,
            this, &QueueMediator::trackAddedAtServer
        );
        connect(
            monitor, &AbstractQueueMonitor::trackRemoved,
            this, &QueueMediator::trackRemovedAtServer
        );
        connect(
            monitor, &AbstractQueueMonitor::trackMoved,
            this, &QueueMediator::trackMovedAtServer
        );
    }

    void QueueMediator::setFetchLimit(int count)
    {
        _sourceMonitor->setFetchLimit(count);
    }

    QUuid QueueMediator::serverUuid() const {
        return _sourceMonitor->serverUuid();
    }

    quint32 QueueMediator::queueEntry(int index) {
        if (index < 0 || index >= _myQueue.size()) {
            /* make sure the info will be fetched from the server */
            (void)(_sourceMonitor->queueEntry(index));

            /* but we HAVE TO return 0 here */
            return 0;
        }

        return _myQueue[index];
    }

    bool QueueMediator::isFetchCompleted() const
    {
        return _sourceMonitor->isFetchCompleted();
    }

    void QueueMediator::removeTrack(int index, quint32 queueID) {
        doLocalOperation(new DeleteOperation(index, queueID));
    }

    void QueueMediator::moveTrack(int fromIndex, int toIndex, quint32 queueID) {
        doLocalOperation(new MoveOperation(fromIndex, toIndex, queueID));
    }

    void QueueMediator::moveTrackToEnd(int fromIndex, quint32 queueId) {
        auto toIndex = queueLength() - 1;
        moveTrack(fromIndex, toIndex, queueId);
    }

    void QueueMediator::insertFileAsync(int index, const FileHash& hash)
    {
        queueController().insertQueueEntryAtIndex(hash, index);
    }

    void QueueMediator::duplicateEntryAsync(quint32 queueID)
    {
        queueController().duplicateQueueEntry(queueID);
    }

    bool QueueMediator::canDuplicateEntry(quint32 queueID) const
    {
        return queueController().canDuplicateEntry(queueID);
    }

    void QueueMediator::resetQueue(int queueLength) {
        qDebug() << "QueueMediator: resetting state, length=" << queueLength;
        _queueLength = queueLength;
        _myQueue = _sourceMonitor->knownQueuePart();

        qDeleteAll(_pendingOperations);
        _pendingOperations.clear();

        Q_EMIT queueResetted(queueLength);
    }

    void QueueMediator::doResetQueue() {
        qDebug() << "QueueMediator: resetting state to that from source";
        _queueLength = _sourceMonitor->queueLength();
        _myQueue = _sourceMonitor->knownQueuePart();

        qDeleteAll(_pendingOperations);
        _pendingOperations.clear();

        Q_EMIT queueResetted(_queueLength);
    }

    void QueueMediator::entriesReceivedAtServer(int index, QList<quint32> entries) {
        handleServerOperation(new InfoOperation(index, entries));
    }

    void QueueMediator::trackAddedAtServer(int index, quint32 queueID) {
        handleServerOperation(new AddOperation(index, queueID));
    }

    void QueueMediator::trackRemovedAtServer(int index, quint32 queueID) {
        handleServerOperation(new DeleteOperation(index, queueID));
    }

    void QueueMediator::trackMovedAtServer(int fromIndex, int toIndex, quint32 queueID) {
        handleServerOperation(new MoveOperation(fromIndex, toIndex, queueID));
    }

    QueueController& QueueMediator::queueController() const
    {
        return _clientServerInterface->queueController();
    }

    bool QueueMediator::doLocalOperation(Operation* op) {
        if (!op->execute(*this, true)) {
            qDebug() << "QueueMediator::doLocalOperation: FAILED to execute";

            /* PROBLEM */
            delete op;

            doResetQueue();
            return false;
        }

        _pendingOperations.append(op);
        return true;
    }

    bool QueueMediator::handleServerOperation(Operation* op) {
        if (_pendingOperations.empty()) {
            bool result = op->execute(*this, false);
            delete op;
            return result;
        }

        if (_pendingOperations.first()->equals(op)) {
            /* already executed */
            Operation* firstOp = _pendingOperations.first();
            _pendingOperations.removeFirst();
            delete firstOp;
            delete op;
            return false;
        }

        /* can't merge operations (yet); rollback everything first */
        qDebug() << "QueueMediator: doing rollback";

        bool allFailMustReset = false;
        for(int i = _pendingOperations.size() - 1; i >= 0; --i) {
            Operation* pastOp = _pendingOperations[i];

            if (!allFailMustReset) {
                allFailMustReset = !pastOp->rollback(*this);
            }

            delete pastOp;
        }

        _pendingOperations.clear();

        /* rollback complete; sanity check */
        if (_queueLength != _sourceMonitor->queueLength()) {
            allFailMustReset = true;
        }

        /* now apply the operation that came from the server */
        if (!allFailMustReset && !op->execute(*this, false)) {
            /* still a problem */
            allFailMustReset = true;
        }

        delete op;

        if (!allFailMustReset) return true;

        qDebug() << " rollback failed; resetting";

        /* failure --> start over fresh */
        doResetQueue();
        return false;
    }

}
