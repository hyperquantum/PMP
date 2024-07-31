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

#ifndef PMP_HISTORYSTATISTICS_H
#define PMP_HISTORYSTATISTICS_H

#include "common/future.h"
#include "common/newfuture.h"
#include "common/nullable.h"
#include "common/resultorerror.h"

#include "trackstats.h"

#include <QDateTime>
#include <QHash>
#include <QMutex>
#include <QObject>
#include <QSet>
#include <QVector>

QT_FORWARD_DECLARE_CLASS(QThreadPool)

namespace PMP::Server
{
    class Database;
    class HashRelations;
    class UserHashStatsCache;

    class HistoryStatistics : public QObject
    {
        Q_OBJECT
    public:
        HistoryStatistics(QObject* parent, HashRelations* hashRelations,
                          UserHashStatsCache* userHashStatsCache);
        ~HistoryStatistics();

        Future<SuccessType, FailureType> addToHistory(quint32 userId,
                                                      quint32 hashId,
                                                      QDateTime start,
                                                      QDateTime end,
                                                      int permillage,
                                                      bool validForScoring);

        Nullable<TrackStats> getStatsIfAvailable(quint32 userId, uint hashId);
        NewFuture<SuccessType, FailureType> scheduleFetchIfMissing(quint32 userId,
                                                                uint hashId);
        void invalidateAllGroupStatisticsForHash(uint hashId);
        void invalidateIndividualHashStatistics(quint32 userId, uint hashId);

    Q_SIGNALS:
        void hashStatisticsChanged(quint32 userId, QVector<uint> hashIds);

    private:
        bool recalculateGroupStats(quint32 userId,
                                   QHash<uint, TrackStats> individualStats);
        void scheduleStatisticsChangedSignal(quint32 userId, QVector<uint> hashIds);

        NewFuture<SuccessType, FailureType> scheduleFetch(quint32 userId, uint hashId,
                                                       bool onlyIfMissing);

        enum class UseCachedValues { Yes, No };

        struct UserHashStatisticsEntry
        {
            Nullable<TrackStats> groupStats;
        };

        struct UserStatisticsEntry
        {
            QHash<uint, UserHashStatisticsEntry> hashData;
            QSet<uint> hashesInProgress;
        };

        static SuccessOrFailure fetchInternal(HistoryStatistics* calculator,
                                              quint32 userId,
                                              QVector<uint> hashIdsInGroup,
                                            UseCachedValues cacheUseForIndividualHashes);

        static ResultOrError<QHash<uint, TrackStats>, FailureType> fetchIndividualStats(
                                                UserHashStatsCache* cache,
                                                quint32 userId,
                                                QVector<uint> hashIdsInGroup,
                                            UseCachedValues cacheUseForIndividualHashes);

        QThreadPool* const _threadPool;
        HashRelations* const _hashRelations;
        UserHashStatsCache* const _userHashStatsCache;
        QMutex _mutex;
        QHash<quint32, UserStatisticsEntry> _userData;
    };
}
#endif
