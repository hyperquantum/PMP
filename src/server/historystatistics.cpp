/*
    Copyright (C) 2022, Kevin Andre <hyperquantum@gmail.com>

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

#include "database.h"
#include "hashrelations.h"

#include <QThreadPool>

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
    }

    HistoryStatistics::HistoryStatistics(QObject* parent, HashRelations* hashRelations)
     : QObject(parent),
       _threadPool(new QThreadPool(this)),
       _hashRelations(hashRelations)
    {
        _threadPool->setMaxThreadCount(2);
    }

    HistoryStatistics::~HistoryStatistics()
    {
        _threadPool->clear();
        _threadPool->waitForDone();
    }

    Future<SuccessType, FailureType> HistoryStatistics::addToHistory(quint32 userId,
                                                                     quint32 hashId,
                                                                     QDateTime start,
                                                                     QDateTime end,
                                                                     int permillage,
                                                                     bool validForScoring)
    {
        auto future =
            Concurrent::run<SuccessType, FailureType>(
                /* do not specify a thread pool, it cannot wait */
                [this, userId, hashId, start, end, permillage, validForScoring]()
                    -> ResultOrError<SuccessType, FailureType>
                {
                    auto db = Database::getDatabaseForCurrentThread();
                    if (!db)
                        return failure;

                    auto added = db->addToHistory(hashId, userId, start, end, permillage,
                                                  validForScoring);
                    if (added.failed())
                        return failure;

                    const auto hashesInGroup =
                            _hashRelations->getEquivalencyGroup(hashId);

                    auto result = fetchInternal(this, userId, hashesInGroup);
                    return result.toSuccessOrFailure();
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

        Concurrent::run<TrackStats, FailureType>(
            _threadPool,
            [this, userId, hashesInGroup]()
            {
                return fetchInternal(this, userId, hashesInGroup);
            }
        );

        return null;
    }

    Future<SuccessType, FailureType> HistoryStatistics::scheduleFetchIfMissing(
                                                                           quint32 userId,
                                                                           uint hashId)
    {
        return scheduleFetch(userId, hashId, true);
    }

    void HistoryStatistics::invalidateStatisticsForHashes(QVector<uint> hashIds)
    {
        QMutexLocker lock(&_mutex);

        qDebug() << "HistoryStatistics: invalidating statistics for hashes:" << hashIds;

        QHash<quint32, QVector<uint>> usersWithHashes;

        for (auto userIt = _userData.begin(); userIt != _userData.end(); ++userIt)
        {
            auto userId = userIt.key();
            auto& userEntry = userIt.value();

            for (auto hashId : hashIds)
            {
                if (userEntry.hashData.remove(hashId) > 0)
                {
                    usersWithHashes[userId].append(hashId);
                }
            }
        }

        lock.unlock();

        for (auto userIt = usersWithHashes.constBegin();
             userIt != usersWithHashes.constEnd();
             ++userIt)
        {
            auto userId = userIt.key();
            auto& hashIds = userIt.value();

            for (auto hashId : hashIds)
                scheduleFetch(userId, hashId, false);
        }
    }

    Future<SuccessType, FailureType> HistoryStatistics::scheduleFetch(quint32 userId,
                                                                      uint hashId,
                                                                      bool onlyIfMissing)
    {
        QMutexLocker lock(&_mutex);

        auto& userData = _userData[userId];
        if (userData.hashesInProgress.contains(hashId))
            return FutureError(failure);

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
                return FutureResult(success); /* nothing to do */
        }

        for (auto hashId : hashesInGroup)
            userData.hashesInProgress << hashId;

        auto future =
            Concurrent::run<TrackStats, FailureType>(
                _threadPool,
                [this, userId, hashesInGroup]()
                {
                    return fetchInternal(this, userId, hashesInGroup);
                }
            );

        return future.toTypelessFuture();
    }

    ResultOrError<TrackStats, FailureType> HistoryStatistics::fetchInternal(
                                                            HistoryStatistics* calculator,
                                                            quint32 userId,
                                                            QVector<uint> hashIdsInGroup)
    {
        qDebug() << "HistoryStatistics: starting fetch for user" << userId
                 << "and hash IDs" << hashIdsInGroup;

        auto db = Database::getDatabaseForCurrentThread();
        if (!db)
            return failure;

        auto historyStatsOrFailure = db->getHashHistoryStats(userId, hashIdsInGroup);
        if (historyStatsOrFailure.failed())
            return failure;

        const auto multipleHistoryStats = historyStatsOrFailure.result();

        QVector<TrackStats> allIndividual;
        allIndividual.reserve(hashIdsInGroup.size());

        QMutexLocker lock(&calculator->_mutex);
        auto& userData = calculator->_userData[userId];

        for (auto& historyStats : multipleHistoryStats)
        {
            auto& hashData = userData.hashData[historyStats.hashId];
            hashData.individualStats = toTrackStats(historyStats);
        }

        for (auto hashId : qAsConst(hashIdsInGroup))
        {
            allIndividual.append(userData.hashData[hashId].individualStats);
        }

        auto calculatedGroupStats = TrackStats::combined(allIndividual);

        for (auto hashId : qAsConst(hashIdsInGroup))
        {
            userData.hashData[hashId].groupStats = calculatedGroupStats;
            userData.hashesInProgress.remove(hashId);
        }

        lock.unlock();

        qDebug() << "HistoryStatistics: group stats (re)calculated for user" << userId
                 << "and hash IDs" << hashIdsInGroup
                 << ": last history ID:" << calculatedGroupStats.lastHistoryId()
                 << " last heard:" << calculatedGroupStats.lastHeard()
                 << " permillage:" << calculatedGroupStats.getScoreOr(-1);

        QTimer::singleShot(
            0, calculator,
            [calculator, userId, hashIdsInGroup]()
            {
                Q_EMIT calculator->hashStatisticsChanged(userId, hashIdsInGroup);
            }
        );

        return calculatedGroupStats;
    }
}
