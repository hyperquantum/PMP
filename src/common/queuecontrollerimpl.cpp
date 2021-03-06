/*
    Copyright (C) 2020-2021, Kevin Andre <hyperquantum@gmail.com>

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

#include "serverconnection.h"

namespace PMP {

    QueueControllerImpl::QueueControllerImpl(ServerConnection* connection)
     : QueueController(connection),
       _connection(connection)
    {
        connect(
            _connection, &ServerConnection::connected,
            this, &QueueControllerImpl::connected
        );
        connect(
            _connection, &ServerConnection::connectionBroken,
            this, &QueueControllerImpl::connectionBroken
        );

        connect(
            _connection, &ServerConnection::queueEntryAdded,
            this, &QueueControllerImpl::queueEntryAdded
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
        return _connection->serverSupportsQueueEntryDuplication();
    }

    void QueueControllerImpl::insertBreakAtFront()
    {
        _connection->insertBreakAtFront();
    }

    void QueueControllerImpl::insertQueueEntryAtFront(FileHash hash)
    {
        _connection->insertQueueEntryAtFront(hash);
    }

    void QueueControllerImpl::insertQueueEntryAtEnd(FileHash hash)
    {
        _connection->insertQueueEntryAtEnd(hash);
    }

    void QueueControllerImpl::insertQueueEntryAtIndex(FileHash hash, quint32 index)
    {
        _connection->insertQueueEntryAtIndex(hash, index);
    }

    void QueueControllerImpl::deleteQueueEntry(uint queueId)
    {
        _connection->deleteQueueEntry(queueId);
    }

    void QueueControllerImpl::duplicateQueueEntry(uint queueId)
    {
        _connection->duplicateQueueEntry(queueId);
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
