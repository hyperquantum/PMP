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

#include "queue.h"

#include "queueentry.h"

namespace PMP {

    Queue::Queue()
     : _nextQueueID(1)
    {
        //
    }

    void Queue::clear() {
        _queue.clear();
    }

    bool Queue::empty() const {
        return _queue.empty();
    }

    uint Queue::length() const {
        return _queue.length();
    }

    QueueEntry* Queue::enqueue(QString const& filename) {
        uint queueID = _nextQueueID++;
        QueueEntry* entry = new QueueEntry(queueID, filename);
        _index.insert(queueID, entry);
        _queue.enqueue(entry);
        return entry;
    }

    QueueEntry* Queue::enqueue(FileData const& filedata) {
        uint queueID = _nextQueueID++;
        QueueEntry* entry = new QueueEntry(queueID, filedata);
        _index.insert(queueID, entry);
        _queue.enqueue(entry);
        return entry;
    }

    QueueEntry* Queue::enqueue(HashID const& hash) {
        uint queueID = _nextQueueID++;
        QueueEntry* entry = new QueueEntry(queueID, hash);
        _index.insert(queueID, entry);
        _queue.enqueue(entry);
        return entry;
    }

    QueueEntry* Queue::dequeue() {
        if (_queue.empty()) { return 0; }
        QueueEntry* entry = _queue.dequeue();

        emit entryRemoved(0, entry->queueID());

        return entry;
    }

    QList<QueueEntry*> Queue::entries(int startoffset, int maxCount) {
        return _queue.mid(startoffset, maxCount);
    }

    QueueEntry* Queue::lookup(quint32 queueID) {
        QHash<quint32, QueueEntry*>::iterator it = _index.find(queueID);
        if (it == _index.end()) { return 0; }

        return it.value();
    }

}
