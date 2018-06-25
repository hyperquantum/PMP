/*
    Copyright (C) 2014-2018, Kevin Andre <hyperquantum@gmail.com>

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

#include <QDateTime>
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

    class Queue : public QObject {
        Q_OBJECT
    public:
        enum HistoryType {
            Played, Skipped, Error
        };

        Queue(Resolver* resolver);

        bool checkPotentialRepetitionByAdd(const FileHash& hash,
                                           int repetitionAvoidanceSeconds,
                                           int* nonRepetitionSpan = nullptr) const;

        uint getNextQueueID();

    public slots:
        bool empty() const;
        uint length() const;

        void clear(bool doNotifications);
        void trim(uint length);

        void enqueue(QueueEntry* entry);
        QueueEntry* enqueue(QString const& filename);
        QueueEntry* enqueue(FileHash const& hash);

        void insertAtFront(QueueEntry* entry);
        QueueEntry* insertAtFront(FileHash const& hash);
        void insertBreakAtFront();

        void insertAtIndex(quint32 index, QueueEntry* entry);
        QueueEntry* insertAtIndex(quint32 index, FileHash const& hash);

        QueueEntry* dequeue();
        bool remove(quint32 queueID);
        bool removeAtIndex(uint index);
        bool move(quint32 queueID, qint16 indexDiff);

        QList<QueueEntry*> entries(int startoffset, int maxCount);

        QueueEntry* lookup(quint32 queueID);

        void addToHistory(QSharedPointer<const PlayerHistoryEntry> entry);
        QList<QSharedPointer<const PlayerHistoryEntry>> recentHistory(int limit);

    Q_SIGNALS:
        void entryAdded(quint32 offset, quint32 queueID);
        void entryRemoved(quint32 offset, quint32 queueID);
        void entryMoved(quint32 fromOffset, quint32 toOffset, quint32 queueID);

    private slots:
        void checkFrontOfQueue();

    private:
        int findIndex(quint32 queueID);

        uint _nextQueueID;
        QHash<quint32, QueueEntry*> _idLookup;
        QQueue<QueueEntry*> _queue;
        QQueue<QSharedPointer<const PlayerHistoryEntry>> _history;
        Resolver* _resolver;
        QTimer* _queueFrontChecker;
    };

    class PlayerHistoryEntry {
    public:
        PlayerHistoryEntry(uint queueID, uint user, QDateTime started, QDateTime ended,
                           bool hadError, bool hadSeek, int permillage)
         : _queueID(queueID), _user(user), _started(started), _ended(ended),
           _permillage(permillage), _error(hadError), _seek(hadSeek)
        {
            //
        }

        uint queueID() const { return _queueID; }
        uint user() const { return _user; }
        QDateTime started() const { return _started; }
        QDateTime ended() const { return _ended; }
        bool hadError() const { return _error; }
        bool hadSeek() const { return _seek; }
        int permillage() const { return _permillage; }

    private:
        uint _queueID;
        uint _user;
        QDateTime _started;
        QDateTime _ended;
        int _permillage;
        bool _error;
        bool _seek;
    };

}
#endif
