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

#include <QtDebug>

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
        return enqueue(new QueueEntry(_nextQueueID++, filename));
    }

    QueueEntry* Queue::enqueue(FileData const& filedata) {
        return enqueue(new QueueEntry(_nextQueueID++, filedata));
    }

    QueueEntry* Queue::enqueue(HashID const& hash) {
        return enqueue(new QueueEntry(_nextQueueID++, hash));
    }

    QueueEntry* Queue::enqueue(QueueEntry* entry) {
        _index.insert(entry->queueID(), entry);
        _queue.enqueue(entry);

        emit entryAdded(_queue.size() - 1, entry->queueID());

        return entry;
    }

    QueueEntry* Queue::dequeue() {
        if (_queue.empty()) { return 0; }
        QueueEntry* entry = _queue.dequeue();

        emit entryRemoved(0, entry->queueID());

        return entry;
    }

    bool Queue::remove(quint32 queueID) {
        int index = findIndex(queueID);
        if (index < 0) return false; // not found

        _queue.removeAt(index);

        emit entryRemoved(index, queueID);

        return true;
    }

    QList<QueueEntry*> Queue::entries(int startoffset, int maxCount) {
        return _queue.mid(startoffset, maxCount);
    }

    QueueEntry* Queue::lookup(quint32 queueID) {
        QHash<quint32, QueueEntry*>::iterator it = _index.find(queueID);
        if (it == _index.end()) { return 0; }

        return it.value();
    }

    int Queue::findIndex(quint32 queueID) {
        // FIXME: find a more efficient way to get the index
        int length = _queue.length();
        for (int i = 0; i < length; ++i) {
            if (_queue[i]->queueID() == queueID) return i;
        }

        return -1; // not found
    }

    bool Queue::checkPotentialRepetitionByAdd(Resolver& resolver, const HashID& hash, int repetitionAvoidanceSeconds, int* nonRepetitionSpan) const {
        int span = 0;

        for (int i = _queue.length() - 1; i >= 0; --i) {
            QueueEntry* entry = _queue[i];
            if (entry->hash() == 0) {
                /* TODO: small problem; we don't know the track's hash yet. This could potentially lead to a repetition. */
                qDebug() << "queue repetition check: don't know hash yet of queue entry" << entry->queueID();
                continue;
            }

            const HashID& entryHash = *entry->hash();

            if (entryHash == hash) {
                if (nonRepetitionSpan != 0) { *nonRepetitionSpan = span; }
                return true; /* potential repetition */
            }

            entry->checkAudioData(resolver);
            int entryLength = entry->lengthInSeconds();

            if (entryLength > 0) {
                span += entryLength;
                if (span >= repetitionAvoidanceSeconds) { break; }
            }
        }

        if (nonRepetitionSpan != 0) { *nonRepetitionSpan = span; }
        return false;
    }

}
