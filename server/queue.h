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
#include <QtGlobal>

QT_FORWARD_DECLARE_CLASS(QTimer)

namespace PMP {

    class FileData;
    class HashID;
    class QueueEntry;
    class Resolver;

    class Queue : public QObject {
        Q_OBJECT
    public:
        enum HistoryType {
            Played, Skipped, Error
        };

        Queue(Resolver* resolver);

        bool checkPotentialRepetitionByAdd(const HashID& hash,
                                           int repetitionAvoidanceSeconds,
                                           int* nonRepetitionSpan = 0) const;

        uint getNextQueueID();

    public slots:
        void clear();
        bool empty() const;
        uint length() const;

        QueueEntry* enqueue(QString const& filename);
        QueueEntry* enqueue(FileData const& filedata);
        QueueEntry* enqueue(HashID const& hash);
        QueueEntry* enqueue(QueueEntry* entry);

        QueueEntry* dequeue();

        bool remove(quint32 queueID);

        QList<QueueEntry*> entries(int startoffset, int maxCount);

        QueueEntry* lookup(quint32 queueID);

        void addToHistory(QueueEntry* entry/*, HistoryType historyType*/);

    Q_SIGNALS:
        void entryAdded(quint32 offset, quint32 queueID);
        void entryRemoved(quint32 offset, quint32 queueID);

    private slots:
        void checkFrontOfQueue();

    private:
        int findIndex(quint32 queueID);

        uint _nextQueueID;
        QHash<quint32, QueueEntry*> _idLookup;
        QQueue<QueueEntry*> _queue;
        QQueue<QueueEntry*> _history;
        Resolver* _resolver;
        QTimer* _queueFrontChecker;
    };
}
#endif
