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

#include "history.h"

#include "addtohistorytask.h"
#include "player.h"
#include "queueentry.h"
#include "resolver.h"
#include "userdatafortracksfetcher.h"

#include <QtDebug>
#include <QThreadPool>
#include <QTimer>

namespace PMP {

    namespace {
        static const int fetchingTimerFreqMs = 20;
    }

    History::History(Player* player)
     : _player(player),
       _nowPlaying(0),
       _fetchRequestsScheduledCount(0),
       _fetchTimerRunning(false)
    {
        connect(
            player, &Player::currentTrackChanged,
            this, &History::currentTrackChanged
        );
        connect(
            player, &Player::newHistoryEntry,
            this, &History::newHistoryEntry
        );
    }

    QDateTime History::lastPlayed(FileHash const& hash) const {
        return _lastPlayHash[hash];
    }

    bool History::fetchMissingUserStats(uint hashID, quint32 user) {
        if (hashID == 0 || _userStats[user].contains(hashID)) return false;

        scheduleFetch(hashID, user);
        return true;
    }

    History::HashStats const* History::getUserStats(uint hashID, quint32 user) {
        if (hashID == 0) return nullptr;

        QHash<uint, HashStats>& userData = _userStats[user];

        auto hashDataIterator = userData.find(hashID);
        if (hashDataIterator != userData.end()) {
            return &hashDataIterator.value();
        }

        /* we will need to fetch the data from the database first */
        scheduleFetch(hashID, user);

        return nullptr;
    }

    void History::scheduleFetch(uint hashID, quint32 user)
    {
        _fetchRequestsScheduledCount++;

        QHash<uint, bool>& hashes = _userStatsFetching[user];

        auto hashDataIterator = hashes.find(hashID);
        if (hashDataIterator == hashes.end()) {
            hashes[hashID] = false; /* add as not yet fetched */
        }

        /*if (_fetchRequestsScheduledCount >= 10) {
            // don't wait, start right now
            sendFetchRequests();
        }
        else */
        if (!_fetchTimerRunning) {
            _fetchTimerRunning = true;
            QTimer::singleShot(
                fetchingTimerFreqMs,
                this,
                [this]() {
                    _fetchTimerRunning = false;
                    sendFetchRequests();
                }
            );
        }
    }

    void History::sendFetchRequests() {
        Q_FOREACH(auto user, _userStatsFetching.keys()) {
            QHash<uint, bool>& hashes = _userStatsFetching[user];

            QVector<uint> hashesToFetch;
            hashesToFetch.reserve(hashes.size());
            Q_FOREACH(auto hash, hashes.keys()) {
                bool& beingFetched = hashes[hash];
                if (!beingFetched) {
                    hashesToFetch.append(hash);
                    beingFetched = true;
                }
            }

            /* nothing to fetch for this user? */
            if (hashesToFetch.isEmpty()) continue;

            /* something to fetch */

            auto fetcher = new UserDataForTracksFetcher(user, hashesToFetch);
            connect(
                fetcher, &UserDataForTracksFetcher::finishedWithResult,
                this, &History::onFetchCompleted
            );
            QThreadPool::globalInstance()->start(fetcher);
        }

        _fetchRequestsScheduledCount = 0;
    }

    void History::currentTrackChanged(QueueEntry const* newTrack) {
        if (_nowPlaying != 0 && newTrack != _nowPlaying) {
            const FileHash* hash = _nowPlaying->hash();
            /* TODO: make sure hash is known, so history won't get lost */
            if (hash) {
                _lastPlayHash[*hash] = QDateTime::currentDateTimeUtc();
            }
        }

        _nowPlaying = newTrack;
    }

    void History::newHistoryEntry(QSharedPointer<PlayerHistoryEntry> entry) {
        if (entry->permillage() <= 0 && entry->hadError())
            return;

        FileHash hash = entry->hash();
        if (hash.isNull()) {
            qWarning() << "cannot save history for queue ID" << entry->queueID()
                       << "because hash is unavailabe";
            return;
        }

        auto task = new AddToHistoryTask(&_player->resolver(), entry);

        connect(
            task, &AddToHistoryTask::updatedHashUserStats,
            this, &History::onHashUserStatsUpdated
        );

        QThreadPool::globalInstance()->start(task);
    }

    void History::onHashUserStatsUpdated(uint hashID, quint32 user,
                                         QDateTime previouslyHeard, qint16 score)
    {
        HashStats& existingStats = _userStats[user][hashID];

        if (previouslyHeard == existingStats.lastHeard
                && score == existingStats.score)
        {
            return; /* no change */
        }

        existingStats.lastHeard = previouslyHeard;
        existingStats.score = score;

        Q_EMIT updatedHashUserStats(hashID, user, previouslyHeard, score);
    }

    void History::onFetchCompleted(quint32 userId, QVector<UserDataForHashId> results)
    {
        QHash<uint, bool>& hashesForFetching = _userStatsFetching[userId];
        QHash<uint, HashStats>& statsForUser = _userStats[userId];

        Q_FOREACH(UserDataForHashId const& hashData, results) {
            auto hashId = hashData.hashId;

            hashesForFetching.remove(hashId);

            HashStats& stats = statsForUser[hashId];

            if (stats.lastHeard != hashData.previouslyHeard
                    || stats.score != hashData.score)
            {
                stats.lastHeard = hashData.previouslyHeard;
                stats.score = hashData.score;

                Q_EMIT updatedHashUserStats(hashId, userId, stats.lastHeard, stats.score);
            }
        }
    }

}
