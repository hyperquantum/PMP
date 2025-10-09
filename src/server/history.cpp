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

#include "history.h"

#include "historystatistics.h"
#include "player.h"
#include "queueentry.h"

#include <QtDebug>
#include <QThreadPool>
#include <QTimer>

namespace PMP::Server
{
    History::History(Player* player, HistoryStatistics* historyStatistics)
     : _player(player),
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

    QDateTime History::lastPlayedGloballySinceStartup(uint trackId) const
    {
        return _lastPlayByTrack[trackId];
    }

    Future<SuccessType, FailureType> History::scheduleUserStatsFetchingIfMissing(
                                                                           uint trackId,
                                                                           quint32 userId)
    {
        if (trackId == 0)
        {
            qWarning() << "History: invalid parameter(s): track ID" << trackId
                       << " user ID" << userId;
            return FutureError(failure);
        }

        return _statistics->scheduleFetchIfMissing(userId, trackId);
    }

    Nullable<TrackStats> History::getUserStats(uint trackId, quint32 userId)
    {
        if (trackId == 0)
        {
            qWarning() << "History: got request for user stats of track ID zero";
            return null;
        }

        return _statistics->getStatsIfAvailable(userId, trackId);
    }

    void History::currentTrackChanged(QSharedPointer<QueueEntry const> newTrack)
    {
        if (_nowPlaying != nullptr && newTrack != _nowPlaying)
        {
            Nullable<uint> trackId = _nowPlaying->trackId();
            if (trackId.hasValue())
            {
                _lastPlayByTrack[trackId.value()] = QDateTime::currentDateTimeUtc();
            }
        }

        _nowPlaying = newTrack;
    }

    void History::newHistoryEntry(QSharedPointer<RecentHistoryEntry> entry)
    {
        if (entry->permillage() <= 0 && entry->hadError())
            return;

        _statistics->addToHistory(entry->user(), entry->trackId(),
                                  entry->started(), entry->ended(), entry->permillage(),
                                  entry->validForScoring());
    }
}
