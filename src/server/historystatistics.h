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

#ifndef PMP_HISTORYSTATISTICS_H
#define PMP_HISTORYSTATISTICS_H

#include "common/resultorerror.h"

#include "databaserecords.h"
#include "trackstats.h"

#include <QDateTime>
#include <QMutex>
#include <QObject>
#include <QSet>
#include <QVector>

namespace PMP
{
    class HashRelations;

    class HistoryStatistics : public QObject
    {
        Q_OBJECT
    public:
        HistoryStatistics(QObject* parent, HashRelations* hashRelations);

        /// Add an entry to the history, update statistics, and return the IDs of the
        /// hashes whose statistics have changed.
        ResultOrError<SuccessType, FailureType> addToHistory(quint32 userId,
                                                             quint32 hashId,
                                                             QDateTime start,
                                                             QDateTime end,
                                                             int permillage,
                                                             bool validForScoring);

        bool wasFetchedAlready(quint32 userId, uint hashId);
        Nullable<TrackStats> tryGet(quint32 userId, uint hashId);

        ResultOrError<TrackStats, FailureType> getOrFetch(quint32 userId, uint hashId);

    Q_SIGNALS:
        void hashStatisticsChanged(quint32 userId, QVector<uint> hashIds);

    private:
        struct UserHashStatisticsEntry
        {
            UserHashStatisticsEntry() : userId(0), hashId(0) {}

            bool updateIndividualStatsWithNewerOnes(TrackStats const& maybeNewerStats)
            {
                if (individualStats.isNull())
                {
                    individualStats = maybeNewerStats;
                    return true;
                }

                return individualStats.value().updateWithNewerStats(maybeNewerStats);
            }

            bool updateMissingIndividualStatsToNeverPlayed()
            {
                if (!individualStats.isNull())
                    return false;

                individualStats = TrackStats{};
                return true;
            }

            void setIndividualStatsAsGroupStats()
            {
                groupStats = individualStats;
            }

            void setGroupStats(TrackStats& stats)
            {
                groupStats = stats;
            }

            quint32 userId;
            uint hashId;
            Nullable<TrackStats> individualStats;
            Nullable<TrackStats> groupStats;
        };

        struct UserStatisticsEntry
        {
            QHash<uint, UserHashStatisticsEntry> hashData;
            //QSet<uint> hashesToFetch;
        };

        /// Fetch statistics for a specific user and hash. Will also fetch statistics
        /// for related hashes if missing. Returns a list of hashes whose statistics have
        /// changed.
        ResultOrError<QVector<uint>, FailureType> fetch(quint32 userId, uint hashId);

        /// From a list of specified hashes, return the subset of hashes whose statistics
        /// are still missing.
        QVector<uint> getHashesWithMissingStats(quint32 userId, QSet<uint> hashIds);

        /// Apply the hash stats that have been fetched from the database, and return a
        /// list of the hashes whose data have been changed as a result.
        QVector<uint> applyFetchedStats(quint32 userId,
                                        QVector<uint> const& hashesToFetch,
                           QVector<DatabaseRecords::HashHistoryStats> const& fetchResult);

        void recalculateGroupStats(quint32 userId, QVector<uint> hashIds);
        ResultOrError<SuccessType, FailureType> fetchMissingRelatedStats(
                                                       UserHashStatisticsEntry& hashData);
        UserHashStatisticsEntry& getOrCreateHashEntry(quint32 userId, uint hashId);

        HashRelations* _hashRelations;
        QMutex _mutex;
        QHash<quint32, UserStatisticsEntry> _userData;
    };
}
#endif
