/*
    Copyright (C) 2020, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_QUEUECONTROLLERIMPL_H
#define PMP_QUEUECONTROLLERIMPL_H

#include "queuecontroller.h"

namespace PMP {

    class ServerConnection;

    class QueueControllerImpl : public QueueController
    {
        Q_OBJECT
    public:
        QueueControllerImpl(ServerConnection* connection);

        bool canDuplicateEntry(quint32 queueId) const override;

    public Q_SLOTS:
        void insertBreakAtFront() override;
        void insertQueueEntryAtFront(FileHash hash) override;
        void insertQueueEntryAtEnd(FileHash hash) override;
        void insertQueueEntryAtIndex(FileHash hash, quint32 index) override;
        void deleteQueueEntry(uint queueId) override;
        void duplicateQueueEntry(uint queueId) override;
        void moveQueueEntry(uint queueId, qint16 offsetDiff) override;

    private Q_SLOTS:
        void connected();
        void connectionBroken();

    private:
        ServerConnection* _connection;
    };
}
#endif
