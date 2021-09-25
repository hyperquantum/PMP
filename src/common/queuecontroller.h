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

#ifndef PMP_QUEUECONTROLLER_H
#define PMP_QUEUECONTROLLER_H

#include "filehash.h"

#include <QObject>

namespace PMP {

    class QueueController : public QObject
    {
        Q_OBJECT
    public:
        virtual ~QueueController() {}

        virtual bool canDuplicateEntry(quint32 queueId) const = 0;

    public Q_SLOTS:
        virtual void insertBreakAtFront() = 0;
        virtual void insertQueueEntryAtFront(FileHash hash) = 0;
        virtual void insertQueueEntryAtEnd(FileHash hash) = 0;
        virtual void insertQueueEntryAtIndex(FileHash hash, quint32 index) = 0;
        virtual void deleteQueueEntry(uint queueId) = 0;
        virtual void duplicateQueueEntry(uint queueId) = 0;
        virtual void moveQueueEntry(uint queueId, qint16 offsetDiff) = 0;

    Q_SIGNALS:
        void queueEntryAdded(qint32 index, quint32 queueId);
        void queueEntryRemoved(qint32 index, quint32 queueId);
        void queueEntryMoved(qint32 fromIndex, qint32 toIndex, quint32 queueId);

    protected:
        explicit QueueController(QObject* parent) : QObject(parent) {}
    };
}
#endif
