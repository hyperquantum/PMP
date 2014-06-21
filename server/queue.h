/*
    Copyright (C) 2014, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_QUEUE_H
#define PMP_QUEUE_H

#include <QHash>
#include <QObject>
#include <QQueue>

namespace PMP {

    class FileData;
    class HashID;
    class QueueEntry;

    class Queue : public QObject {
        Q_OBJECT
    public:
        Queue();

    public slots:
        void clear();
        bool empty() const;
        uint length() const;

        QueueEntry* enqueue(QString const& filename);
        QueueEntry* enqueue(FileData const& filedata);
        QueueEntry* enqueue(HashID const& hash);

        QueueEntry* dequeue();

        QList<QueueEntry*> entries(int startoffset, int maxCount);

        QueueEntry* lookup(quint32 queueID);

    Q_SIGNALS:
        //void entryAdded(quint32 offset, quint32 queueID);
        void entryRemoved(quint32 offset, quint32 queueID);

    private:
        uint _nextQueueID;
        QHash<quint32, QueueEntry*> _index;
        QQueue<QueueEntry*> _queue;
    };
}
#endif