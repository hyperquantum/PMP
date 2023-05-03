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

#include "trackstats.h"

namespace PMP::Server
{
    TrackStats TrackStats::fromHistory(uint lastHistoryId, QDateTime lastHeard,
                                       quint32 scoreHeardCount, qint16 averagePermillage)
    {
        return
            TrackStats
            {
                lastHistoryId,
                lastHeard,
                scoreHeardCount,
                averagePermillage,
                calculateScore(averagePermillage, scoreHeardCount)
            };
    }

    TrackStats TrackStats::combined(QVector<TrackStats> individualStatsList)
    {
        if (individualStatsList.empty())
            return {};

        if (individualStatsList.size() == 1)
            return individualStatsList[0];

        uint groupLastHistoryId = 0;
        uint groupScoreHeardCount = 0;
        double groupScoresSum = 0;
        QDateTime groupLastHeard;

        for (auto& individualStats : qAsConst(individualStatsList))
        {
            if (individualStats._lastHistoryId <= 0)
                continue;

            double individualScoresSum = double(individualStats._averagePermillage)
                                            * individualStats._scoreHeardCount;

            if (groupScoreHeardCount == 0)
            {
                groupLastHistoryId = individualStats.lastHistoryId();
                groupScoreHeardCount = individualStats._scoreHeardCount;
                groupScoresSum = individualScoresSum;
                groupLastHeard = individualStats.lastHeard();
                continue;
            }

            if (!individualStats.lastHeard().isNull()
                    && groupLastHeard < individualStats.lastHeard())
            {
                groupLastHeard = individualStats.lastHeard();
            }

            if (groupLastHistoryId < individualStats.lastHistoryId())
                groupLastHistoryId = individualStats.lastHistoryId();

            groupScoresSum += individualScoresSum;
            groupScoreHeardCount += individualStats._scoreHeardCount;
        }

        double groupAveragePermillage = -1;
        if (groupScoreHeardCount > 0)
            groupAveragePermillage = groupScoresSum / groupScoreHeardCount;

        int groupAveragePermillageInt = static_cast<int>(groupAveragePermillage);

        auto combinedStats =
                TrackStats::fromHistory(groupLastHistoryId, groupLastHeard,
                                        groupScoreHeardCount,
                                        static_cast<qint16>(groupAveragePermillageInt));

        return combinedStats;
    }
}
