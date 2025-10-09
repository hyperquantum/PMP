/*
    Copyright (C) 2014-2025, Kevin André <hyperquantum@gmail.com>

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

#include "playerqueue.h"

#include "hashidregistrar.h"
#include "queueentry.h"
#include "resolver.h"

#include <QtDebug>
#include <QTimer>

namespace PMP::Server
{
    namespace
    {
         /* this limit could be increased or decreased in the future */
        const int maximumQueueLength = 2'000'000;
    }

    PlayerQueue::PlayerQueue(HashIdRegistrar* hashIdRegistrar, Resolver* resolver)
     : _hashIdRegistrar(hashIdRegistrar), _resolver(resolver),
        _queueFrontChecker(new QTimer(this)),
        _nextQueueID(1), _firstTrackIndex(-1), _firstTrackQueueId(0)
    {
        connect(
            _queueFrontChecker, &QTimer::timeout,
            this, &PlayerQueue::checkFrontOfQueue
        );

        _queueFrontChecker->start(10 * 1000);
    }

    void PlayerQueue::checkFrontOfQueue()
    {
        int length = _queue.length();
        uint operationsDone = 0;

        for (int index = 0; index < length && index < 10 && operationsDone <= 3; ++index)
        {
            auto entry = _queue[index];
            if (!entry->isTrack())
                continue;

            auto trackId = entry->trackId().value();
            auto filename = entry->filename();
            if (filename.hasValue()
                && !_resolver->pathStillValid(trackId, filename.value()))
            {
                qDebug() << "PlayerQueue: filename no longer valid for queue index"
                         << (index + 1);
                filename = null;
                entry->invalidateFilename();
            }

            int& backoff = entry->fileFinderBackoff();

            if (filename.hasValue())
            {
                backoff = 0;
                continue;
            }

            if (backoff > 0)
            {
                backoff--;
                continue;
            }

            qDebug() << "PlayerQueue: need to obtain a valid filename for queue index"
                     << (index + 1) << "which has queue ID" << entry->queueID()
                     << "and track ID" << trackId;

            backoff = 10;
            auto future = _resolver->findPathForTrackAsync(trackId);
            future.handleOnEventLoop(
                this,
                [entry, index](FailureOr<QString> outcome)
                {
                    if (outcome.succeeded())
                    {
                        auto path = outcome.result();

                        qDebug() << "PlayerQueue: found file" << path
                                 << "for queue ID" << entry->queueID();

                        entry->fileFinderBackoff() = 0;
                        entry->fileFinderFailedCount() /= 2;
                        entry->setFilename(path);
                    }
                    else
                    {
                        int& failedCount = entry->fileFinderFailedCount();
                        failedCount = qMin(failedCount + 1, 100);
                        entry->fileFinderBackoff() = failedCount + (index * 2);
                    }
                }
            );
        }
    }

    int PlayerQueue::toIndex(QueueInsertionPosition position)
    {
        if (position == QueueInsertionPosition::Front)
            return 0;
        else //if (position == QueueInsertionPosition::End)
            return _queue.length();
    }

    bool PlayerQueue::empty() const
    {
        return _queue.empty();
    }

    int PlayerQueue::length() const
    {
        return _queue.length();
    }

    bool PlayerQueue::canAddMoreEntries(int count) const
    {
        if (count < 0) return false;
        if (count == 0) return true;

        return _queue.length() <= maximumQueueLength - count;
    }

    uint PlayerQueue::getNextQueueID()
    {
        return _nextQueueID++;
    }

    void PlayerQueue::resetFirstTrack()
    {
        _firstTrackIndex = -1;
        _firstTrackQueueId = 0;
    }

    void PlayerQueue::setFirstTrackIndexAndId(int index, uint queueId)
    {
        _firstTrackIndex = index;
        _firstTrackQueueId = queueId;
    }

    void PlayerQueue::findFirstTrackBetweenIndices(int start, int end,
                                                   bool resetIfNoneFound)
    {
        for (int i = start; i < end && i < _queue.length(); ++i)
        {
            auto entry = _queue[i];
            if (!entry->isTrack())
                continue;

            setFirstTrackIndexAndId(i, entry->queueID());
            return;
        }

        if (resetIfNoneFound)
            resetFirstTrack();
    }

    void PlayerQueue::emitFirstTrackChanged()
    {
        qDebug() << "first track changed; index:" << _firstTrackIndex
                 << " id:" << _firstTrackQueueId;

        Q_EMIT firstTrackChanged(_firstTrackIndex, _firstTrackQueueId);
    }

    void PlayerQueue::trim(int length)
    {
        while (_queue.length() > length)
        {
            removeAtIndex(_queue.length() - 1);
        }
    }

    QList<ResultOrError<QueueEntryIdsAndHash, class Error>>
        PlayerQueue::getHashAndTrackIdForQueueIds(QList<uint> queueIds) const
    {
        QList<ResultOrError<QueueEntryIdsAndHash, class Error>> result;
        result.reserve(queueIds.size());

        for (auto queueId : queueIds)
        {
            auto it = _idLookup.find(queueId);
            if (it == _idLookup.constEnd())
            {
                result.append(Error::queueEntryIdNotFound(queueId));
                continue;
            }

            result.append(toQueueEntryIdsAndHash(it.value()));
        }

        return result;
    }

    QueueEntryIdsAndHash PlayerQueue::toQueueEntryIdsAndHash(
        QSharedPointer<QueueEntry> entry) const
    {
        Nullable<FileHashWithId> hashAndId;

        if (entry->isTrack())
        {
            auto trackId = entry->trackId().value();
            auto hash = _hashIdRegistrar->getHashForId(trackId).value();

            hashAndId = FileHashWithId(hash, trackId);
        }

        return
            {
                .queueId = entry->queueID(),
                .kind = entry->kind(),
                .hashAndId = hashAndId,
            };
    }

    Result PlayerQueue::enqueue(uint trackId)
    {
        return insertTrack(QueueInsertionPosition::End, trackId);
    }

    Result PlayerQueue::enqueue(FileHash hash)
    {
        return insertTrack(QueueInsertionPosition::End, hash);
    }

    Result PlayerQueue::insertAtFront(uint trackId)
    {
        return insertTrack(QueueInsertionPosition::Front, trackId);
    }

    Result PlayerQueue::insertAtFront(FileHash hash)
    {
        return insertTrack(QueueInsertionPosition::Front, hash);
    }

    Result PlayerQueue::insertBreakAtFront()
    {
        return insertAtIndex(0, QueueEntryCreators::breakpoint());
    }

    Result PlayerQueue::insertTrack(QueueInsertionPosition position, FileHash hash)
    {
        if (hash.isNull())
            return Error::hashIsNull();

        auto trackId = _hashIdRegistrar->getIdForHash(hash);
        if (trackId == null)
            return Error::hashIsUnknown();

        auto index = toIndex(position);

        return insertAtIndex(index, QueueEntryCreators::track(trackId.value()));
    }

    Result PlayerQueue::insertTrack(QueueInsertionPosition position, uint trackId)
    {
        if (trackId == 0)
            return Error::trackIdIsZero();

        if (_hashIdRegistrar->isRegisteredId(trackId) == false)
            return Error::trackIdIsUnknown();

        auto index = toIndex(position);

        return insertAtIndex(index, QueueEntryCreators::track(trackId));
    }

    Result PlayerQueue::insertAtIndex(qint32 index,
                       std::function<QSharedPointer<QueueEntry> (uint)> queueEntryCreator)
    {
        return insertAtIndex(index, queueEntryCreator, [](uint) {});
    }

    Result PlayerQueue::insertAtIndex(qint32 index, SpecialQueueItemType itemType,
                                      std::function<void (uint)> queueIdNotifier)
    {
        std::function<QSharedPointer<QueueEntry> (uint)> queueEntryCreator;

        switch (itemType)
        {
        case PMP::SpecialQueueItemType::Break:
            queueEntryCreator = QueueEntry::createBreak;
            break;

        case PMP::SpecialQueueItemType::Barrier:
            queueEntryCreator = QueueEntry::createBarrier;
            break;

        default:
            return Error::queueItemTypeInvalid();
        }

        return insertAtIndex(index, queueEntryCreator, queueIdNotifier);
    }

    Result PlayerQueue::insertAtIndex(qint32 index, uint trackId,
                                      std::function<void (uint)> queueIdNotifier)
    {
        if (trackId == 0)
            return Error::trackIdIsZero();

        if (_hashIdRegistrar->isRegisteredId(trackId) == false)
            return Error::trackIdIsUnknown();

        auto entryCreator = QueueEntryCreators::track(trackId);

        return insertAtIndex(index, entryCreator, queueIdNotifier);
    }

    Result PlayerQueue::insertAtIndex(qint32 index, FileHash hash,
                                      std::function<void (uint)> queueIdNotifier)
    {
        if (hash.isNull())
            return Error::hashIsNull();

        auto trackId = _hashIdRegistrar->getIdForHash(hash);
        if (trackId == null)
            return Error::hashIsUnknown();

        auto entryCreator = QueueEntryCreators::track(trackId.value());

        return insertAtIndex(index, entryCreator, queueIdNotifier);
    }

    Result PlayerQueue::duplicateEntryWithId(uint queueId,
                                             std::function<void (uint)> queueIdNotifier)
    {
        auto index = findIndex(queueId);
        if (index < 0)
            return Error::queueEntryIdNotFound(queueId);

        auto existing = _queue.at(index);
        if (existing->queueID() != queueId)
        {
            qWarning() << "queue inconsistency for QID" << queueId;
            return Error::internalError();
        }

        auto entryCreator = QueueEntryCreators::copyOf(existing);

        return insertAtIndex(index + 1, entryCreator, queueIdNotifier);
    }

    Result PlayerQueue::insertAtIndex(qint32 index,
                       std::function<QSharedPointer<QueueEntry> (uint)> queueEntryCreator,
                       std::function<void (uint)> queueIdNotifier)
    {
        if (index < 0
                || index > _queue.size()) /* notice: one past the end is allowed */
        {
            qWarning() << "queue index out of range:" << index;
            return Error::queueIndexOutOfRange();
        }

        if (!canAddMoreEntries())
        {
            qWarning() << "queue does not allow adding another entry";
            return Error::queueMaxSizeExceeded();
        }

        auto id = getNextQueueID();
        auto entry = queueEntryCreator(id);
        if (entry->queueID() != id)
        {
            qWarning() << "new queue entry did not adopt the specified queue ID";
            return Error::internalError();
        }

        _idLookup.insert(entry->queueID(), entry);
        _queue.insert(int(index), entry);

        bool firstTrackChange = true;
        if ((_firstTrackIndex < 0 || _firstTrackIndex >= index) && entry->isTrack())
            setFirstTrackIndexAndId(index, entry->queueID());
        else if (_firstTrackIndex >= 0 && _firstTrackIndex >= index)
            _firstTrackIndex++;
        else
            firstTrackChange = false;

        queueIdNotifier(entry->queueID());
        Q_EMIT entryAdded(index, entry->queueID());

        if (firstTrackChange)
            emitFirstTrackChanged();

        return Success();
    }

    QSharedPointer<QueueEntry> PlayerQueue::dequeue()
    {
        if (_queue.empty()) { return nullptr; }
        auto entry = _queue.dequeue();

        bool firstTrackChange = true;
        if (_firstTrackIndex < 0)
            firstTrackChange = false;
        else if (_firstTrackIndex == 0)
            findFirstTrackBetweenIndices(0, _queue.length(), true);
        else
            _firstTrackIndex--;

        Q_EMIT entryRemoved(0, entry->queueID());

        if (firstTrackChange)
            emitFirstTrackChanged();

        return entry;
    }

    bool PlayerQueue::remove(quint32 queueID)
    {
        int index = findIndex(queueID);
        if (index < 0) return false; // not found

        return removeAtIndex(index);
    }

    bool PlayerQueue::removeAtIndex(int index)
    {
        if (index < 0 || index >= _queue.length())
            return false;

        auto entry = _queue[index];
        quint32 queueID = entry->queueID();
        _queue.removeAt(index);

        bool firstTrackChange = true;
        if (_firstTrackIndex < 0 || _firstTrackIndex < index)
            firstTrackChange = false;
        else if (_firstTrackIndex == index)
            findFirstTrackBetweenIndices(index, _queue.length(), true);
        else
            _firstTrackIndex--;

        Q_EMIT entryRemoved(index, queueID);

        qDebug() << "deleting QID" << queueID
                 << "from lookup table because it was deleted from the queue";

        _idLookup.remove(queueID);

        if (firstTrackChange)
            emitFirstTrackChanged();

        return true;
    }

    bool PlayerQueue::moveById(quint32 queueID, qint16 indexDiff)
    {
        int index = findIndex(queueID);
        if (index < 0) return false; // not found

        return moveByIndex(index, indexDiff);
    }

    bool PlayerQueue::moveByIndex(int index, qint16 indexDiff)
    {
        if (index < 0 || index >= _queue.length())
            return false; /* invalid index */

        if (indexDiff == 0) return true; /* no-op */

        auto entry = _queue[index];
        auto queueId = entry->queueID();

        /* sanity checks */
        if (indexDiff < 0)
        {
            /* cannot move beyond first place */
            if (index < -indexDiff)
            {
                qDebug() << "Queue::move: cannot move item" << queueId << "upwards"
                         << -indexDiff << "places because its index is now" << index;
                return false;
            }
        }
        else /* indexDiff > 0 */
        {
            /* cannot move beyond last place */
            if (_queue.size() - 1 - indexDiff < index)
            {
                qDebug() << "Queue::move: cannot move item" << queueId << "downwards"
                         << indexDiff << "places because its index is now" << index
                         << "and the queue only has" << _queue.size() << "items";
                return false;
            }
        }

        int newIndex = index + indexDiff;
        _queue.move(index, newIndex);

        bool firstTrackChange = true;
        if (_firstTrackIndex < index && _firstTrackIndex < newIndex)
            firstTrackChange = false;
        else if (_firstTrackIndex > index && _firstTrackIndex > newIndex)
            firstTrackChange = false;
        /* else if (_firstTrackIndex < 0)  this is already covered by the first if
            firstTrackChange = false; */
        else if (newIndex < index) /* -- track moved up */
        {
            if (entry->isTrack())
                setFirstTrackIndexAndId(newIndex, queueId);
            else
                _firstTrackIndex++; /* moved down to make room */
        }
        else /* newIndex > index -- track moved down */
        {
            if (_firstTrackIndex == index)
                findFirstTrackBetweenIndices(index, newIndex + 1, true);
            else
                _firstTrackIndex--; /* moved up to make room */
        }

        Q_EMIT entryMoved(index, newIndex, queueId);

        if (firstTrackChange)
            emitFirstTrackChanged();

        return true;
    }

    QList<QSharedPointer<QueueEntry>> PlayerQueue::entries(int startoffset, int maxCount)
    {
        return _queue.mid(startoffset, maxCount);
    }

    QSharedPointer<QueueEntry> PlayerQueue::peek() const
    {
        return entryAtIndex(0);
    }

    bool PlayerQueue::firstEntryIsBarrier() const
    {
        auto firstEntry = peek();

        return firstEntry && firstEntry->kind() == QueueEntryKind::Barrier;
    }

    QSharedPointer<QueueEntry> PlayerQueue::peekFirstTrackEntry() const
    {
        if (_firstTrackIndex < 0)
            return nullptr;

        return _queue[_firstTrackIndex];
    }

    QSharedPointer<QueueEntry> PlayerQueue::lookup(quint32 queueID)
    {
        auto it = _idLookup.find(queueID);
        if (it == _idLookup.end()) { return nullptr; }

        return it.value();
    }

    QSharedPointer<QueueEntry> PlayerQueue::entryAtIndex(int index) const
    {
        if (index < 0 || index >= _queue.size())
            return nullptr;

        return _queue.at(index);
    }

    void PlayerQueue::addToHistory(QSharedPointer<RecentHistoryEntry> entry)
    {
        if (!entry) return;

        qDebug() << "adding QID" << entry->queueID()
                 << "to the queue history; play-permillage:" << entry->permillage()
                 << " error?" << entry->hadError();
        _history.enqueue(entry);

        if (_history.size() > 20)
        {
            auto oldest = _history.dequeue();
            qDebug() << "deleting oldest queue history entry: QID" << oldest->queueID();

            auto oldestEntry = _idLookup[oldest->queueID()];
            _idLookup.remove(oldest->queueID());
        }

        qDebug() << " history size now:" << _history.size();
    }

    QList<QSharedPointer<RecentHistoryEntry>> PlayerQueue::recentHistory(int limit)
    {
        if (limit <= 0 || limit > _history.size())
            return _history;

        return _history.mid(_history.size() - limit, limit);
    }

    int PlayerQueue::findIndex(quint32 queueID)
    {
        if (queueID <= 0)
            return -1;

        // FIXME: find a more efficient way to get the index
        int length = _queue.length();
        for (int i = 0; i < length; ++i)
        {
            if (_queue[i]->queueID() == queueID) return i;
        }

        return -1; // not found
    }

    TrackRepetitionInfo PlayerQueue::checkPotentialRepetitionByAdd(uint trackId,
                                                     int repetitionAvoidanceSeconds,
                                                     qint64 extraMarginMilliseconds) const
    {
        qint64 millisecondsCounted = extraMarginMilliseconds;

        for (int i = _queue.length() - 1; i >= 0; --i)
        {
            auto entry = _queue[i];
            if (!entry->isTrack())
                continue;

            auto entryTrackId = entry->trackId().value();

            if (entryTrackId == trackId)
                return TrackRepetitionInfo(true, millisecondsCounted);

            entry->checkAudioData(*_resolver);
            qint64 entryLengthMilliseconds = entry->lengthInMilliseconds();

            if (entryLengthMilliseconds > 0)
            {
                millisecondsCounted += entryLengthMilliseconds;

                if (millisecondsCounted >= repetitionAvoidanceSeconds * qint64(1000))
                {
                    break; /* time between the tracks is large enough */
                }
            }
        }

        return TrackRepetitionInfo(false, millisecondsCounted);
    }
}
