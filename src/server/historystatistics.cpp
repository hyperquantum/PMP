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

#include "historystatistics.h"

#include "common/containerutil.h"

#include "database.h"
#include "hashrelations.h"

using PMP::DatabaseRecords::HashHistoryStats;

namespace PMP
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
       _hashRelations(hashRelations)
    {
        //
    }

    ResultOrError<SuccessType, FailureType> HistoryStatistics::addToHistory(
                                                                    quint32 userId,
                                                                    quint32 hashId,
                                                                    QDateTime start,
                                                                    QDateTime end,
                                                                    int permillage,
                                                                    bool validForScoring)
    {
        auto db = Database::getDatabaseForCurrentThread();
        if (!db)
            return failure;

        auto added = db->addToHistory(hashId, userId, start, end, permillage,
                                      validForScoring);
        if (added.failed())
            return failure;

        QMutexLocker lock(&_mutex);

        auto maybeUpdatedHashes = fetch(userId, hashId);

        if (maybeUpdatedHashes.failed())
            return failure;

        Q_EMIT hashStatisticsChanged(userId, maybeUpdatedHashes.result());

        return success;
    }

    bool HistoryStatistics::wasFetchedAlready(quint32 userId, uint hashId)
    {
        QMutexLocker lock(&_mutex);

        auto& hashEntry = getOrCreateHashEntry(userId, hashId);

        return hashEntry.groupStats.hasValue();
    }

    Nullable<TrackStats> HistoryStatistics::tryGet(quint32 userId, uint hashId)
    {
        QMutexLocker lock(&_mutex);

        auto& hashEntry = getOrCreateHashEntry(userId, hashId);

        if (hashEntry.groupStats.hasValue())
            return hashEntry.groupStats;

        return null;
    }

    ResultOrError<TrackStats, FailureType> HistoryStatistics::getOrFetch(
                                                                        quint32 userId,
                                                                        uint hashId)
    {
        QMutexLocker lock(&_mutex);

        auto& hashEntry = getOrCreateHashEntry(userId, hashId);

        if (hashEntry.groupStats.hasValue())
            return hashEntry.groupStats.value();

        auto maybeUpdatedHashes = fetch(userId, hashId);

        if (maybeUpdatedHashes.failed())
            return failure;

        if (hashEntry.groupStats.isNull()) /* shouldn't happen */
        {
            qWarning() << "HistoryStatistics: stats still missing after successful fetch;"
                       << "user" << userId << " hashId" << hashId;
            return failure;
        }

        Q_EMIT hashStatisticsChanged(userId, maybeUpdatedHashes.result());

        return hashEntry.groupStats.value();
    }

    ResultOrError<QVector<uint>, FailureType> HistoryStatistics::fetch(quint32 userId,
                                                                       uint hashId)
    {
        auto relatedHashIds = _hashRelations->getOtherHashesEquivalentTo(hashId);

        auto relatedHashesToFetch = getHashesWithMissingStats(userId, relatedHashIds);
        auto hashesToFetch = relatedHashesToFetch;
        hashesToFetch << hashId;

        auto db = Database::getDatabaseForCurrentThread();
        if (!db)
            return failure;

        auto historyStatsOrFailure = db->getHashHistoryStats(userId, hashesToFetch);
        if (historyStatsOrFailure.failed())
            return failure;

        auto multipleHistoryStats = historyStatsOrFailure.result();

        //TRY_ASSIGN(multipleHistoryStats, db->getHashHistoryStats(userId, hashesToFetch))

        auto hashesUpdated =
                applyFetchedStats(userId, hashesToFetch, multipleHistoryStats);

        if (hashesUpdated.empty())
            return hashesUpdated;

        // recalculate group stats
        if (relatedHashIds.empty())
        {
            auto& hashData = getOrCreateHashEntry(userId, hashId);
            hashData.setIndividualStatsAsGroupStats();
        }
        else
        {
            auto allHashIds = relatedHashIds;
            allHashIds << hashId;
            recalculateGroupStats(userId, ContainerUtil::toVector(allHashIds));
        }

        return hashesUpdated;
    }

    QVector<uint> HistoryStatistics::getHashesWithMissingStats(quint32 userId,
                                                               QSet<uint> hashIds)
    {
        QVector<uint> result;
        result.reserve(hashIds.size());

        for (auto hashId : hashIds)
        {
            auto& hashEntry = getOrCreateHashEntry(userId, hashId);

            if (hashEntry.individualStats.isNull())
                result.append(hashId);
        }

        return result;
    }

    QVector<uint> HistoryStatistics::applyFetchedStats(quint32 userId,
                                                       const QVector<uint>& hashesToFetch,
                                             QVector<HashHistoryStats> const& fetchResult)
    {
        QVector<uint> hashesUpdated;
        hashesUpdated.reserve(hashesToFetch.size());

        for (auto const& historyStats : fetchResult)
        {
            auto hashId = historyStats.hashId;
            auto& hashData = getOrCreateHashEntry(userId, hashId);
            auto stats = toTrackStats(historyStats);

            qDebug() << "HistoryStatistics: applying stats for user" << userId
                     << "and hash ID" << hashId
                     << "; last history ID:" << historyStats.lastHistoryId
                     << "; avg. permillage:" << historyStats.averagePermillage
                     << "; last heard:" << historyStats.lastHeard;

            bool updated = hashData.updateIndividualStatsWithNewerOnes(stats);
            if (updated)
                hashesUpdated.append(hashId);
        }

        for (auto hashIdToFetch : hashesToFetch)
        {
            auto& hashData = getOrCreateHashEntry(userId, hashIdToFetch);

            qDebug() << "HistoryStatistics: no playback history present for user"
                     << userId << "and hash ID" << hashIdToFetch;

            bool updated = hashData.updateMissingIndividualStatsToNeverPlayed();
            if (updated)
                hashesUpdated.append(hashIdToFetch);
        }

        return hashesUpdated;
    }

    void HistoryStatistics::recalculateGroupStats(quint32 userId, QVector<uint> hashIds)
    {
        QVector<TrackStats> trackStatsList;
        trackStatsList.reserve(hashIds.size());

        for (auto hashId : hashIds)
        {
            auto& hashData = getOrCreateHashEntry(userId, hashId);
            if (hashData.individualStats.isNull())
                continue;

            auto individualStats = hashData.individualStats.value();
            trackStatsList.append(individualStats);
        }

        auto newGroupStats = TrackStats::combined(trackStatsList);

        qDebug() << "HistoryStatistics: group stats (re)calculated for user" << userId
                 << "and hash IDs" << hashIds
                 << ": last history ID:" << newGroupStats.lastHistoryId()
                 << " last heard:" << newGroupStats.lastHeard()
                 << " permillage:" << newGroupStats.getScoreOr(-1);

        for (auto hashId : hashIds)
        {
            getOrCreateHashEntry(userId, hashId).setGroupStats(newGroupStats);
        }
    }

    HistoryStatistics::UserHashStatisticsEntry& HistoryStatistics::getOrCreateHashEntry(
                                                                           quint32 userId,
                                                                           uint hashId)
    {
        auto& userData = _userData[userId];
        auto& hashData = userData.hashData[hashId];
        hashData.userId = userId;
        hashData.hashId = hashId;

        return hashData;
    }
}
