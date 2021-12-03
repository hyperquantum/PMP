/*
    Copyright (C) 2014-2021, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_PLAYERQUEUE_H
#define PMP_PLAYERQUEUE_H

#include "common/filehash.h"
#include "common/specialqueueitemtype.h"

#include "playerhistoryentry.h"
#include "result.h"

#include <QHash>
#include <QObject>
#include <QQueue>
#include <QSharedPointer>
#include <QtGlobal>

#include <functional>

QT_FORWARD_DECLARE_CLASS(QTimer)

namespace PMP
{
    class FileHash;
    class PlayerHistoryEntry;
    class QueueEntry;
    class Resolver;

    class TrackRepetitionInfo
    {
    public:
        TrackRepetitionInfo(bool isRepetition, qint64 millisecondsCounted)
         : _millisecondsCounted(millisecondsCounted), _isRepetition(isRepetition)
        {
            //
        }

        bool isRepetition() const { return _isRepetition; }
        qint64 millisecondsCounted() const { return _millisecondsCounted; }

    private:
        qint64 _millisecondsCounted;
        bool _isRepetition;
    };

    class PlayerQueue : public QObject
    {
        Q_OBJECT
    public:
        enum HistoryType
        {
            Played, Skipped, Error
        };

        PlayerQueue(Resolver* resolver);

        TrackRepetitionInfo checkPotentialRepetitionByAdd(FileHash hash,
                                                          int repetitionAvoidanceSeconds,
                                                          qint64 extraMarginMilliseconds
                                                          ) const;

        uint getNextQueueID();

        bool empty() const;
        uint length() const;
        bool canAddMoreEntries(int count = 1) const;

        int firstTrackIndex() const { return _firstTrackIndex; }
        uint firstTrackQueueId() const { return _firstTrackQueueId; }

        QueueEntry* peek() const;
        bool firstEntryIsBarrier() const;
        QueueEntry* peekFirstTrackEntry() const;
        QueueEntry* lookup(quint32 queueID);
        int findIndex(quint32 queueID);
        QueueEntry* entryAtIndex(int index) const;
        QList<QueueEntry*> entries(int startoffset, int maxCount);

        Result enqueue(QString const& filename);
        Result enqueue(FileHash hash);
        Result enqueue(std::function<QueueEntry* (uint)> queueEntryCreator);

        Result insertAtFront(FileHash hash);
        Result insertBreakAtFront();
        Result insertAtFront(std::function<QueueEntry* (uint)> queueEntryCreator);

        Result insertAtIndex(qint32 index, FileHash hash);
        Result insertAtIndex(qint32 index,
                             std::function<QueueEntry* (uint)> queueEntryCreator);
        Result insertAtIndex(qint32 index, SpecialQueueItemType itemType,
                             std::function<void (uint)> queueIdNotifier);
        Result insertAtIndex(qint32 index,
                             std::function<QueueEntry* (uint)> queueEntryCreator,
                             std::function<void (uint)> queueIdNotifier);

        QList<QSharedPointer<PlayerHistoryEntry> > recentHistory(int limit);

    public Q_SLOTS:
        //void clear(bool doNotifications);
        void trim(uint length);

        QueueEntry* dequeue();
        bool remove(quint32 queueID);
        bool removeAtIndex(uint index);
        bool moveById(quint32 queueID, qint16 indexDiff);
        bool moveByIndex(int index, qint16 indexDiff);

        void addToHistory(QSharedPointer<PlayerHistoryEntry> entry);

    Q_SIGNALS:
        void entryAdded(quint32 offset, quint32 queueID);
        void entryRemoved(quint32 offset, quint32 queueID);
        void entryMoved(quint32 fromOffset, quint32 toOffset, quint32 queueID);

        void firstTrackChanged(int index, uint queueId);

    private Q_SLOTS:
        void checkFrontOfQueue();

    private:
        void resetFirstTrack();
        void setFirstTrackIndexAndId(int index, uint queueId);
        void findFirstTrackBetweenIndices(int start, int end, bool resetIfNoneFound);
        void emitFirstTrackChanged();

        uint _nextQueueID;
        int _firstTrackIndex;
        uint _firstTrackQueueId;
        QHash<quint32, QueueEntry*> _idLookup;
        QQueue<QueueEntry*> _queue;
        QQueue<QSharedPointer<PlayerHistoryEntry>> _history;
        Resolver* _resolver;
        QTimer* _queueFrontChecker;
    };
}
#endif
