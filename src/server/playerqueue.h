/*
    Copyright (C) 2014-2020, Kevin Andre <hyperquantum@gmail.com>

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

#include "common/filehash.h"

#include "playerhistoryentry.h"

#include <QHash>
#include <QObject>
#include <QQueue>
#include <QSharedPointer>
#include <QtGlobal>

QT_FORWARD_DECLARE_CLASS(QTimer)

namespace PMP {

    class FileHash;
    class PlayerHistoryEntry;
    class QueueEntry;
    class Resolver;

    class PlayerQueue : public QObject {
        Q_OBJECT
    public:
        enum HistoryType {
            Played, Skipped, Error
        };

        PlayerQueue(Resolver* resolver);

        bool checkPotentialRepetitionByAdd(FileHash hash,
                                           int repetitionAvoidanceSeconds,
                                           int* nonRepetitionSpan = nullptr) const;

        uint getNextQueueID();

        bool empty() const;
        uint length() const;

        QueueEntry* peekFirstTrackEntry(int maxIndex);
        QueueEntry* lookup(quint32 queueID);
        int findIndex(quint32 queueID);
        QueueEntry* entryAtIndex(int index);
        QList<QueueEntry*> entries(int startoffset, int maxCount);

        QList<QSharedPointer<PlayerHistoryEntry> > recentHistory(int limit);

    public slots:
        void clear(bool doNotifications);
        void trim(uint length);

        void enqueue(QueueEntry* entry);
        QueueEntry* enqueue(QString const& filename);
        QueueEntry* enqueue(FileHash hash);

        void insertAtFront(QueueEntry* entry);
        QueueEntry* insertAtFront(FileHash hash);
        void insertBreakAtFront();

        void insertAtIndex(quint32 index, QueueEntry* entry);
        QueueEntry* insertAtIndex(quint32 index, FileHash hash);

        QueueEntry* dequeue();
        bool remove(quint32 queueID);
        bool removeAtIndex(uint index);
        bool move(quint32 queueID, qint16 indexDiff);

        void addToHistory(QSharedPointer<PlayerHistoryEntry> entry);

    Q_SIGNALS:
        void entryAdded(quint32 offset, quint32 queueID);
        void entryRemoved(quint32 offset, quint32 queueID);
        void entryMoved(quint32 fromOffset, quint32 toOffset, quint32 queueID);

    private slots:
        void checkFrontOfQueue();

    private:
        uint _nextQueueID;
        QHash<quint32, QueueEntry*> _idLookup;
        QQueue<QueueEntry*> _queue;
        QQueue<QSharedPointer<PlayerHistoryEntry>> _history;
        Resolver* _resolver;
        QTimer* _queueFrontChecker;
    };
}
#endif