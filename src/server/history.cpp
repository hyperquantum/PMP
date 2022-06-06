/*
    Copyright (C) 2014-2022, Kevin Andre <hyperquantum@gmail.com>

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

#include "history.h"

#include "common/concurrent.h"

#include "hashidregistrar.h"
#include "hashrelations.h"
#include "player.h"
#include "queueentry.h"

#include <QtDebug>
#include <QThreadPool>
#include <QTimer>

namespace PMP
{
    History::History(Player* player, HashIdRegistrar* hashIdRegistrar,
                     HashRelations* hashRelations)
     : _threadPool(new QThreadPool(this)),
       _player(player),
       _hashIdRegistrar(hashIdRegistrar),
       _hashRelations(hashRelations),
       _statistics(new HistoryStatistics(this, hashRelations)),
       _nowPlaying(nullptr)
    {
        _threadPool->setMaxThreadCount(2);

        connect(
            player, &Player::currentTrackChanged,
            this, &History::currentTrackChanged
        );
        connect(
            player, &Player::newHistoryEntry,
            this, &History::newHistoryEntry
        );
        connect(
            _statistics, &HistoryStatistics::hashStatisticsChanged,
            this, &History::hashStatisticsChanged
        );
    }

    History::~History()
    {
        _threadPool->clear();
        _threadPool->waitForDone();
    }

    QDateTime History::lastPlayedGloballySinceStartup(FileHash const& hash) const
    {
        return _lastPlayHash[hash];
    }

    void History::scheduleUserStatsFetchingIfMissing(uint hashId, quint32 userId)
    {
        if (hashId == 0 || userId == 0)
        {
            qWarning() << "History: invalid parameter(s): hashId" << hashId
                       << "user" << userId;
            return;
        }

        if (_statistics->wasFetchedAlready(userId, hashId))
            return;

        scheduleFetch(hashId, userId);
    }

    Nullable<TrackStats> History::getUserStats(uint hashId, quint32 userId)
    {
        if (hashId == 0)
        {
            qWarning() << "History: got request for user stats of hash ID zero";
            return null;
        }

        auto maybeStats = _statistics->tryGet(userId, hashId);
        if (maybeStats.hasValue())
            return maybeStats.value();

        /* we will need to fetch the data from the database first */
        scheduleFetch(hashId, userId);

        return null;
    }

    void History::scheduleFetch(uint hashId, quint32 userId)
    {
        if (_userDataBeingFetched[userId].contains(hashId))
            return; /* already being fetched */

        _userDataBeingFetched[userId] << hashId;

        auto* statistics = _statistics;

        auto future =
            Concurrent::run<TrackStats, FailureType>(
                _threadPool,
                [statistics, userId, hashId]()
                {
                    return statistics->getOrFetch(userId, hashId);
                }
            );

        future.addListener(
            this,
            [this, userId, hashId](ResultOrError<TrackStats, FailureType> result)
            {
                _userDataBeingFetched[userId].remove(hashId);

                if (result.failed())
                {
                    qWarning() << "History: failed to fetch statistics for user" << userId
                               << "and hash ID" << hashId;
                }
            }
        );
    }

    void History::currentTrackChanged(QSharedPointer<QueueEntry const> newTrack)
    {
        if (_nowPlaying != nullptr && newTrack != _nowPlaying)
        {
            Nullable<FileHash> hash = _nowPlaying->hash();
            if (hash.hasValue())
            {
                _lastPlayHash[hash.value()] = QDateTime::currentDateTimeUtc();
            }
        }

        _nowPlaying = newTrack;
    }

    void History::newHistoryEntry(QSharedPointer<PlayerHistoryEntry> entry)
    {
        if (entry->permillage() <= 0 && entry->hadError())
            return;

        FileHash hash = entry->hash();
        if (hash.isNull())
        {
            qWarning() << "cannot save history for queue ID" << entry->queueID()
                       << "because hash is unavailabe";
            return;
        }

        auto* statistics = _statistics;

        _hashIdRegistrar->getOrCreateId(hash)
            .thenAsync<SuccessType, FailureType>(
                [statistics, entry](uint hashId)
                {
                    uint userId = entry->user();
                    bool validForScoring = !(entry->hadError() || entry->hadSeek());

                    return statistics->addToHistory(userId, hashId, entry->started(),
                                                    entry->ended(), entry->permillage(),
                                                    validForScoring);
                },
                failureIdentityFunction
            );
    }
}
