/*
    Copyright (C) 2014-2015, Kevin Andre <hyperquantum@gmail.com>

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
#include "resolver.h"

#include <QtDebug>
#include <QTimer>

namespace PMP {

    Queue::Queue(Resolver* resolver)
     : _nextQueueID(1),
       _resolver(resolver), _queueFrontChecker(new QTimer(this))
    {
        connect(_queueFrontChecker, SIGNAL(timeout()), this, SLOT(checkFrontOfQueue()));
        _queueFrontChecker->start(10 * 1000);
    }

    void Queue::checkFrontOfQueue() {
        int length = _queue.length();
        uint operationsDone = 0;

        for (int i = 0; i < length && i < 10 && operationsDone <= 3; ++i) {
            QueueEntry* entry = _queue[i];
            if (!entry->isTrack()) continue;

            if (entry->hash() == 0) {
                qDebug() << "Queue: need to calculate hash for queue index number" << (i + 1);
                operationsDone++;
                if (!entry->checkHash(*_resolver)) continue; /* check next track */
            }

            QString const* filename = entry->filename();
            if (filename != 0 && !_resolver->pathStillValid(*entry->hash(), *filename)) {
                qDebug() << "Queue: filename no longer valid for queue index number" << (i + 1);
                filename = 0;
            }

            if (filename == 0) {
                int& backoff = entry->fileFinderBackoff();
                if (backoff > 0) {
                    backoff--;
                    continue;
                }

                int& failedCount = entry->fileFinderFailedCount();

                qDebug() << "Queue: need to get a valid filename for queue index number" << (i + 1);
                operationsDone++;
                if (entry->checkValidFilename(*_resolver, false)) {
                    backoff = 0;
                    if (failedCount > 0) failedCount >>= 1; /* divide by two */
                }
                else {
                    failedCount = qMin(failedCount + 1, 100);
                    backoff = failedCount + (i * 2);
                    continue;
                }
            }

            /* TODO: preload files right at the front (here or in the player) */
        }
    }

    void Queue::clear(bool doNotifications) {
        if (doNotifications) {
            trim(0);
            return;
        }

        _queue.clear();
    }

    bool Queue::empty() const {
        return _queue.empty();
    }

    uint Queue::length() const {
        return _queue.length();
    }

    uint Queue::getNextQueueID() {
        return _nextQueueID++;
    }

    void Queue::trim(uint length) {
        while ((uint)_queue.length() > length) {
            removeAtIndex(_queue.length() - 1);
        }
    }

    QueueEntry* Queue::enqueue(QString const& filename) {
        return enqueue(new QueueEntry(this, filename));
    }

    QueueEntry* Queue::enqueue(FileData const& filedata) {
        return enqueue(new QueueEntry(this, filedata));
    }

    QueueEntry* Queue::enqueue(FileHash const& hash) {
        return enqueue(new QueueEntry(this, hash));
    }

    QueueEntry* Queue::enqueue(QueueEntry* entry) {
        _idLookup.insert(entry->queueID(), entry);
        _queue.enqueue(entry);

        emit entryAdded(_queue.size() - 1, entry->queueID());
        return entry;
    }

    QueueEntry* Queue::enqueueAtFront(QueueEntry* entry) {
        _idLookup.insert(entry->queueID(), entry);
        _queue.prepend(entry);

        emit entryAdded(0, entry->queueID());
        return entry;
    }

    void Queue::insertBreakAtFront() {
        if (!_queue.empty() && _queue[0]->type() == QueueEntryType::Break)
            return;

        enqueueAtFront(QueueEntry::createBreak(this));
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

        return removeAtIndex(index);
    }

    bool Queue::removeAtIndex(uint index) {
        if (index >= (uint)_queue.length()) return false;

        QueueEntry* entry = _queue[index];
        quint32 queueID = entry->queueID();
        _queue.removeAt(index);

        emit entryRemoved(index, queueID);

        qDebug() << "deleting QID" << queueID
                 << "from lookup table because it was deleted from the queue";

        _idLookup.remove(queueID);
        delete entry;

        return true;
    }

    bool Queue::move(quint32 queueID, qint16 indexDiff) {
        int index = findIndex(queueID);
        if (index < 0) return false; // not found

        if (indexDiff == 0) return true; /* no-op */

        /* sanity checks */
        if (indexDiff < 0) {
            /* cannot move beyond first place */
            if (index < -indexDiff) {
                qDebug() << "Queue::move: cannot move item" << queueID << "upwards"
                    << -indexDiff << "places because its index is now" << index;
                return false;
            }
        }
        else { /* indexDiff > 0 */
            /* cannot move beyond last place */
            if (_queue.size() - indexDiff < index) {
                qDebug() << "Queue::move: cannot move item" << queueID << "downwards"
                    << indexDiff << "places because its index is now" << index
                    << "and the queue only has" << _queue.size() << "items";
                return false;
            }
        }

        int newIndex = index + indexDiff;
        _queue.move(index, newIndex);

        emit entryMoved(index, newIndex, queueID);
        return true;
    }

    QList<QueueEntry*> Queue::entries(int startoffset, int maxCount) {
        return _queue.mid(startoffset, maxCount);
    }

    QueueEntry* Queue::lookup(quint32 queueID) {
        QHash<quint32, QueueEntry*>::iterator it = _idLookup.find(queueID);
        if (it == _idLookup.end()) { return 0; }

        return it.value();
    }

    void Queue::addToHistory(QueueEntry* entry, int permillagePlayed, bool hadError/*, Queue::HistoryType historyType*/) {
        if (!entry || !entry->isTrack()) return;

        qDebug() << "adding QID" << entry->queueID() << "to the queue history; play-permillage:" << permillagePlayed << " error?" << hadError;
        _history.enqueue(entry);

        if (_history.size() > 10) {
            QueueEntry* oldest = _history.dequeue();
            qDebug() << " deleting QID" << oldest->queueID() << "after removing it from the queue history";

            _idLookup.remove(oldest->queueID());
            delete oldest;
        }

        qDebug() << " history size now:" << _history.size();
    }

    int Queue::findIndex(quint32 queueID) {
        // FIXME: find a more efficient way to get the index
        int length = _queue.length();
        for (int i = 0; i < length; ++i) {
            if (_queue[i]->queueID() == queueID) return i;
        }

        return -1; // not found
    }

    bool Queue::checkPotentialRepetitionByAdd(const FileHash& hash,
                                              int repetitionAvoidanceSeconds,
                                              int* nonRepetitionSpan) const
    {
        int span = 0;

        for (int i = _queue.length() - 1; i >= 0; --i) {
            QueueEntry* entry = _queue[i];
            if (!entry->isTrack()) continue;

            if (entry->hash() == 0) {
                /* we don't know the track's hash yet. We need to calculate it first */
                qDebug() << "Queue::checkPotentialRepetitionByAdd: need to calculate hash first, for QID " << entry->queueID();
                entry->checkHash(*_resolver);

                if (entry->hash() == 0) {
                    qDebug() << "PROBLEM: failed calculating hash of QID " << entry->queueID();
                    /* could not calculate hash, so let's pray that this is a different
                       track and continue */
                    continue;
                }
            }

            const FileHash& entryHash = *entry->hash();

            if (entryHash == hash) {
                if (nonRepetitionSpan != 0) { *nonRepetitionSpan = span; }
                return true; /* potential repetition */
            }

            entry->checkAudioData(*_resolver);
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
