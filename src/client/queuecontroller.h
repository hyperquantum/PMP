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

#ifndef PMP_CLIENT_QUEUECONTROLLER_H
#define PMP_CLIENT_QUEUECONTROLLER_H

#include "common/queueindextype.h"
#include "common/requestid.h"
#include "common/resultmessageerrorcode.h"
#include "common/specialqueueitemtype.h"

#include "client/localhashid.h"

#include <QObject>

namespace PMP::Client
{
    class QueueController : public QObject
    {
        Q_OBJECT
    public:
        virtual ~QueueController() {}

        virtual bool canDuplicateEntry(quint32 queueId) const = 0;
        virtual bool canInsertBreakAtAnyIndex() const = 0;
        virtual bool canInsertBarrier() const = 0;

    public Q_SLOTS:
        virtual void insertBreakAtFrontIfNotExists() = 0;
        virtual void insertQueueEntryAtFront(LocalHashId hashId) = 0;
        virtual void insertQueueEntryAtEnd(LocalHashId hashId) = 0;
        virtual RequestID insertQueueEntryAtIndex(LocalHashId hashId, quint32 index) = 0;
        virtual RequestID insertSpecialItemAtIndex(SpecialQueueItemType itemType,
                                                   int index,
                                   QueueIndexType indexType = QueueIndexType::Normal) = 0;
        virtual void deleteQueueEntry(uint queueId) = 0;
        virtual RequestID duplicateQueueEntry(uint queueId) = 0;
        virtual void moveQueueEntry(uint queueId, qint16 offsetDiff) = 0;

    Q_SIGNALS:
        void queueEntryAdded(qint32 index, quint32 queueId, RequestID requestId);
        void queueEntryInsertionFailed(ResultMessageErrorCode errorCode,
                                       RequestID requestId);
        void queueEntryRemoved(qint32 index, quint32 queueId);
        void queueEntryMoved(qint32 fromIndex, qint32 toIndex, quint32 queueId);

    protected:
        explicit QueueController(QObject* parent) : QObject(parent) {}
    };
}
#endif
