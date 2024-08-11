/*
    Copyright (C) 2022-2024, Kevin Andre <hyperquantum@gmail.com>

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

#include "historystatistics.h"

#include "common/concurrent.h"
#include "common/containerutil.h"

#include "database.h"
#include "hashrelations.h"
#include "userhashstatscache.h"

#include <QThreadPool>
#include <QTimer>

using PMP::Server::DatabaseRecords::HashHistoryStats;

namespace PMP::Server
{
    namespace
    {
        TrackStats toTrackStats(HashHistoryStats const& historyStats)
        {
            auto stats =
                    TrackStats::fromHistory(historyStats.lastHistoryId,
                                            historyStats.lastHeard,
                                            historyStats.scoreHeardCount,
                                            historyStats.averagePermillage);

            return stats;
        }

        QHash<uint, TrackStats> toTrackStats(QVector<HashHistoryStats> historyStats,
                                             int customReserveSize = -1)
        {
            QHash<uint, TrackStats> result;

            result.reserve(customReserveSize >= 0
                               ? customReserveSize
                               : historyStats.size());

            for (auto const& stats : qAsConst(historyStats))
            {
                result.insert(stats.hashId, toTrackStats(stats));
            }

            return result;
        }
    }

    HistoryStatistics::HistoryStatistics(QObject* parent, HashRelations* hashRelations,
                                         UserHashStatsCache* userHashStatsCache)
     : QObject(parent),
       _threadPool(new QThreadPool(this)),
       _hashRelations(hashRelations),
       _userHashStatsCache(userHashStatsCache)
    {
        _threadPool->setMaxThreadCount(2);
    }

    HistoryStatistics::~HistoryStatistics()
    {
        _threadPool->clear();
        _threadPool->waitForDone();
    }

    NewFuture<SuccessType, FailureType> HistoryStatistics::addToHistory(quint32 userId,
                                                                     quint32 hashId,
                                                                     QDateTime start,
                                                                     QDateTime end,
                                                                     int permillage,
                                                                     bool validForScoring)
    {
        auto future =
            Concurrent::runOnThreadPool<SuccessType, FailureType>(
                /* do not specify our own (limited) thread pool, it cannot wait */
                globalThreadPool,
                [this, userId, hashId, start, end, permillage, validForScoring]()
                    -> SuccessOrFailure
                {
                    auto db = Database::getDatabaseForCurrentThread();
                    if (!db)
                        return failure;

                    auto historyIdOrFailure =
                        db->addToHistory(hashId, userId, start, end, permillage,
                                         validForScoring);
                    if (historyIdOrFailure.failed())
                        return failure;

                    uint historyId = historyIdOrFailure.result();

                    const auto hashesInGroup =
                            _hashRelations->getEquivalencyGroup(hashId);

                    auto tryFetch =
                        fetchInternal(this, userId, hashesInGroup, UseCachedValues::No);

                    if (tryFetch.failed())
                        return failure;

                    auto tryToUpdateMiscData =
                        db->updateMiscDataValueFromSpecific("UserHashStatsCacheHistoryId",
                                                        QString::number(historyId - 1),
                                                        QString::number(historyId));

                    return tryToUpdateMiscData;
                }
            );

        return future;
    }

    Nullable<TrackStats> HistoryStatistics::getStatsIfAvailable(quint32 userId,
                                                                uint hashId)
    {
        QMutexLocker lock(&_mutex);

        auto& userData = _userData[userId];
        auto it = userData.hashData.constFind(hashId);
        if (it != userData.hashData.constEnd())
            return it.value().groupStats;

        if (userData.hashesInProgress.contains(hashId))
            return null;

        const auto hashesInGroup = _hashRelations->getEquivalencyGroup(hashId);

        for (auto hashId : hashesInGroup)
            userData.hashesInProgress << hashId;

        Concurrent::runOnThreadPool<SuccessType, FailureType>(
            _threadPool,
            [this, userId, hashesInGroup]()
            {
                return fetchInternal(this, userId, hashesInGroup, UseCachedValues::Yes);
            }
        );

        return null;
    }

    NewFuture<SuccessType, FailureType> HistoryStatistics::scheduleFetchIfMissing(
                                                                           quint32 userId,
                                                                           uint hashId)
    {
        return scheduleFetch(userId, hashId, true);
    }

    void HistoryStatistics::invalidateAllGroupStatisticsForHash(uint hashId)
    {
        QMutexLocker lock(&_mutex);

        qDebug() << "HistoryStatistics: invalidating all group statistics for hash:"
                 << hashId;

        const auto hashesInGroup = _hashRelations->getEquivalencyGroup(hashId);

        QSet<quint32> usersNeedingRefetch;
        usersNeedingRefetch.reserve(_userData.size());

        for (auto userIt = _userData.begin(); userIt != _userData.end(); ++userIt)
        {
            auto userId = userIt.key();
            auto& userEntry = userIt.value();

            auto individualCachedStatsForUser =
                _userHashStatsCache->getForUser(userId, hashesInGroup);

            if (individualCachedStatsForUser.size() == hashesInGroup.size())
            {
                bool changed =
                    recalculateGroupStats(userId,
                                          toTrackStats(individualCachedStatsForUser));

                if (changed)
                    scheduleStatisticsChangedSignal(userId, hashesInGroup);

                continue;
            }

            for (auto hashIdFromGroup : hashesInGroup)
            {
                if (userEntry.hashData.remove(hashIdFromGroup) > 0)
                {
                    usersNeedingRefetch << userId;
                }
            }
        }

        lock.unlock();

        for (auto userId : qAsConst(usersNeedingRefetch))
        {
            scheduleFetch(userId, hashId, false);
        }
    }

    void HistoryStatistics::invalidateIndividualHashStatistics(quint32 userId,
                                                               uint hashId)
    {
        QMutexLocker lock(&_mutex);

        Concurrent::runOnThreadPool<SuccessType, FailureType>(
            _threadPool,
            [this, userId, hashId]() -> SuccessOrFailure
            {
                auto database = Database::getDatabaseForCurrentThread();
                if (!database)
                    return failure;

                auto tryToRemoveFromCacheInDatabase =
                    database->removeUserHashStatsCacheEntry(userId, hashId);

                if (tryToRemoveFromCacheInDatabase.failed())
                    return failure;

                QMutexLocker lock(&_mutex);
                _userHashStatsCache->remove(userId, hashId);
                lock.unlock();

                const auto hashesInGroup = _hashRelations->getEquivalencyGroup(hashId);

                return fetchInternal(this, userId, hashesInGroup, UseCachedValues::Yes);
            }
        );
    }

    bool HistoryStatistics::recalculateGroupStats(quint32 userId,
                                                  QHash<uint, TrackStats> individualStats)
    {
        Q_ASSERT_X(!individualStats.isEmpty(),
                   "HistoryStatistics::recalculateGroupStats",
                   "no individual stats");

        auto newGroupStats =
            TrackStats::combined(ContainerUtil::valuesToVector(individualStats));

        auto& userData = _userData[userId];

        bool haveChanges = false;

        for (auto it = individualStats.constBegin();
             it != individualStats.constEnd();
             ++it)
        {
            auto& hashData = userData.hashData[it.key()];

            haveChanges |=
                hashData.groupStats.isNull()
                           || hashData.groupStats.value() != newGroupStats;

            hashData.groupStats = newGroupStats;
        }

        if (haveChanges)
        {
            qDebug() << "HistoryStatistics: hash group stats changed for user" << userId
                     << "and hash ID" << (*individualStats.keyBegin())
                     << "; group size:" << individualStats.size()
                     << "; last history ID:" << newGroupStats.lastHistoryId()
                     << " last heard:" << newGroupStats.lastHeard()
                     << " permillage:" << newGroupStats.getScoreOr(-1);
        }

        return haveChanges;
    }

    void HistoryStatistics::scheduleStatisticsChangedSignal(quint32 userId,
                                                            QVector<uint> hashIds)
    {
        QTimer::singleShot(
            0,
            this,
            [this, userId, hashIds]() { Q_EMIT hashStatisticsChanged(userId, hashIds); }
        );
    }

    NewFuture<SuccessType, FailureType> HistoryStatistics::scheduleFetch(quint32 userId,
                                                                      uint hashId,
                                                                      bool onlyIfMissing)
    {
        QMutexLocker lock(&_mutex);

        auto& userData = _userData[userId];
        if (userData.hashesInProgress.contains(hashId))
            return NewFutureError(failure);

        const auto hashesInGroup = _hashRelations->getEquivalencyGroup(hashId);

        if (onlyIfMissing)
        {
            bool anyMissing = false;
            for (auto hashId : hashesInGroup)
            {
                if (userData.hashData.contains(hashId))
                    continue;

                anyMissing = true;
                break;
            }

            if (!anyMissing)
                return NewFutureResult(success); /* nothing to do */
        }

        for (auto hashId : hashesInGroup)
            userData.hashesInProgress << hashId;

        auto future =
            Concurrent::runOnThreadPool<SuccessType, FailureType>(
                _threadPool,
                [this, userId, hashesInGroup]()
                {
                    return fetchInternal(this, userId, hashesInGroup,
                                         UseCachedValues::Yes);
                }
            );

        return future;
    }

    SuccessOrFailure HistoryStatistics::fetchInternal(HistoryStatistics* calculator,
                                                      quint32 userId,
                                                      QVector<uint> hashIdsInGroup,
                                            UseCachedValues cacheUseForIndividualHashes)
    {
        qDebug() << "HistoryStatistics: starting fetch for user" << userId
                 << "and hash IDs" << hashIdsInGroup;

        auto* cache = calculator->_userHashStatsCache;

        auto individualStatsOrFailure =
            fetchIndividualStats(cache, userId, hashIdsInGroup,
                                 cacheUseForIndividualHashes);

        if (individualStatsOrFailure.failed())
            return failure;

        auto individualStats = individualStatsOrFailure.result();

        QMutexLocker lock(&calculator->_mutex);
        auto& userData = calculator->_userData[userId];

        bool groupStatsChanged =
            calculator->recalculateGroupStats(userId, individualStats);

        ContainerUtil::removeFromSet(hashIdsInGroup, userData.hashesInProgress);

        lock.unlock();

        if (groupStatsChanged)
            calculator->scheduleStatisticsChangedSignal(userId, hashIdsInGroup);

        return success;
    }

    ResultOrError<QHash<uint, TrackStats>, FailureType>
        HistoryStatistics::fetchIndividualStats(UserHashStatsCache* cache, quint32 userId,
                                                QVector<uint> hashIdsInGroup,
                                            UseCachedValues cacheUseForIndividualHashes)
    {
        QHash<uint, TrackStats> result;

        if (cacheUseForIndividualHashes == UseCachedValues::Yes)
        {
            //ensureCacheHasBeenLoadedForUser(cache, userId);
            auto const statsFromCache = cache->getForUser(userId, hashIdsInGroup);
            result = toTrackStats(statsFromCache, hashIdsInGroup.size());

            if (result.size() == hashIdsInGroup.size())
                return result;
        }
        else
        {
            result.reserve(hashIdsInGroup.size());

            qDebug() << "HistoryStatistics: recalculating for user" << userId
                     << "and hashes" << hashIdsInGroup;
        }

        QSet<uint> toFetch = ContainerUtil::toSet(hashIdsInGroup);
        ContainerUtil::removeKeysFromSet(result, toFetch);

        auto database = Database::getDatabaseForCurrentThread();
        if (!database)
            return failure;

        if (cacheUseForIndividualHashes == UseCachedValues::Yes)
        {
            auto statsFromCacheTable =
                database->getCachedHashStats(userId, ContainerUtil::toVector(toFetch));

            if (statsFromCacheTable.failed())
                return failure;

            for (auto& record : statsFromCacheTable.result())
            {
                cache->add(userId, record);
                result.insert(record.hashId, toTrackStats(record));

                toFetch.remove(record.hashId);
            }

            if (toFetch.isEmpty())
                return result;
        }

        auto statsFromHistoryTable =
            database->getHashHistoryStats(userId, ContainerUtil::toVector(toFetch));

        if (statsFromHistoryTable.failed())
            return failure;

        for (auto& record : statsFromHistoryTable.result())
        {
            cache->add(userId, record);
            result.insert(record.hashId, toTrackStats(record));

            // TODO: should we ignore errors?
            database->updateUserHashStatsCacheEntry(userId, record);
        }

        return result;
    }
}
