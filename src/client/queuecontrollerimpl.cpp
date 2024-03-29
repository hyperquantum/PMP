/*
    Copyright (C) 2020-2023, Kevin Andre <hyperquantum@gmail.com>

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

#include "queuecontrollerimpl.h"

#include "servercapabilities.h"
#include "serverconnection.h"

namespace PMP::Client
{
    QueueControllerImpl::QueueControllerImpl(ServerConnection* connection)
     : QueueController(connection),
       _connection(connection)
    {
        connect(
            _connection, &ServerConnection::connected,
            this, &QueueControllerImpl::connected
        );
        connect(
            _connection, &ServerConnection::disconnected,
            this, &QueueControllerImpl::connectionBroken
        );

        connect(
            _connection, &ServerConnection::queueEntryAdded,
            this, &QueueControllerImpl::queueEntryAdded
        );
        connect(
            _connection, &ServerConnection::queueEntryInsertionFailed,
            this, &QueueControllerImpl::queueEntryInsertionFailed
        );
        connect(
            _connection, &ServerConnection::queueEntryRemoved,
            this, &QueueControllerImpl::queueEntryRemoved
        );
        connect(
            _connection, &ServerConnection::queueEntryMoved,
            this, &QueueControllerImpl::queueEntryMoved
        );
    }

    bool QueueControllerImpl::canDuplicateEntry(quint32 queueId) const
    {
        Q_UNUSED(queueId)

        /* we COULD simulate duplication for tracks on older servers, with a regular
         * insert operation, but there is no reason to put in the effort at this time */
        return _connection->serverCapabilities().supportsQueueEntryDuplication();
    }

    bool QueueControllerImpl::canInsertBreakAtAnyIndex() const
    {
        return _connection->serverCapabilities().supportsInsertingBreaksAtAnyIndex();
    }

    bool QueueControllerImpl::canInsertBarrier() const
    {
        return _connection->serverCapabilities().supportsInsertingBarriers();
    }

    void QueueControllerImpl::insertBreakAtFrontIfNotExists()
    {
        _connection->insertBreakAtFrontIfNotExists();
    }

    void QueueControllerImpl::insertQueueEntryAtFront(LocalHashId hashId)
    {
        _connection->insertQueueEntryAtFront(hashId);
    }

    void QueueControllerImpl::insertQueueEntryAtEnd(LocalHashId hashId)
    {
        _connection->insertQueueEntryAtEnd(hashId);
    }

    RequestID QueueControllerImpl::insertQueueEntryAtIndex(LocalHashId hashId,
                                                           quint32 index)
    {
        return _connection->insertQueueEntryAtIndex(hashId, index);
    }

    RequestID QueueControllerImpl::insertSpecialItemAtIndex(SpecialQueueItemType itemType,
                                                            int index,
                                                            QueueIndexType indexType)
    {
        return _connection->insertSpecialQueueItemAtIndex(itemType, index, indexType);
    }

    void QueueControllerImpl::deleteQueueEntry(uint queueId)
    {
        _connection->deleteQueueEntry(queueId);
    }

    RequestID QueueControllerImpl::duplicateQueueEntry(uint queueId)
    {
        return _connection->duplicateQueueEntry(queueId);
    }

    void QueueControllerImpl::moveQueueEntry(uint queueId, qint16 offsetDiff)
    {
        _connection->moveQueueEntry(queueId, offsetDiff);
    }

    void QueueControllerImpl::connected()
    {
        //
    }

    void QueueControllerImpl::connectionBroken()
    {
        //
    }

}
