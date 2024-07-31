/*
    Copyright (C) 2014-2024, Kevin Andre <hyperquantum@gmail.com>

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

#include "hashidregistrar.h"
#include "historystatistics.h"
#include "player.h"
#include "queueentry.h"

#include <QtDebug>
#include <QThreadPool>
#include <QTimer>

namespace PMP::Server
{
    History::History(Player* player, HashIdRegistrar* hashIdRegistrar,
                     HistoryStatistics* historyStatistics)
     : _player(player),
       _hashIdRegistrar(hashIdRegistrar),
       _statistics(historyStatistics),
       _nowPlaying(nullptr)
    {
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
        //
    }

    QDateTime History::lastPlayedGloballySinceStartup(FileHash const& hash) const
    {
        return _lastPlayHash[hash];
    }

    NewFuture<SuccessType, FailureType> History::scheduleUserStatsFetchingIfMissing(
                                                                           uint hashId,
                                                                           quint32 userId)
    {
        if (hashId == 0)
        {
            qWarning() << "History: invalid parameter(s): hashId" << hashId
                       << "user" << userId;
            return NewFutureError(failure);
        }

        return _statistics->scheduleFetchIfMissing(userId, hashId);
    }

    Nullable<TrackStats> History::getUserStats(uint hashId, quint32 userId)
    {
        if (hashId == 0)
        {
            qWarning() << "History: got request for user stats of hash ID zero";
            return null;
        }

        return _statistics->getStatsIfAvailable(userId, hashId);
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

    void History::newHistoryEntry(QSharedPointer<RecentHistoryEntry> entry)
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
            .thenFuture<SuccessType, FailureType>(
                [statistics, entry](uint hashId)
                {
                    uint userId = entry->user();

                    return statistics->addToHistory(userId, hashId, entry->started(),
                                                    entry->ended(), entry->permillage(),
                                                    entry->validForScoring());
                },
                failureIdentityFunction
            );
    }
}
