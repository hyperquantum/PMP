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

#ifndef PMP_TRACKSTATS_H
#define PMP_TRACKSTATS_H

#include <QDateTime>

namespace PMP::Server
{
    class TrackStats
    {
    public:
        TrackStats()
         : _lastHistoryId(0),
           _scoreHeardCount(0),
           _averagePermillage(-1),
           _score(-1)
        {
            //
        }

        static TrackStats fromHistory(uint lastHistoryId, QDateTime lastHeard,
                                      quint32 scoreHeardCount, qint16 averagePermillage);
        static TrackStats combined(QVector<TrackStats> individualStatsList);

        constexpr uint lastHistoryId() const { return _lastHistoryId; }
        QDateTime lastHeard() const { return _lastHeard; }
        constexpr qint16 score() const { return _score; }

        constexpr bool haveScore() const { return score() >= 0; }

        constexpr int getScoreOr(int alternative)
        {
            return haveScore() ? score() : alternative;
        }

        constexpr bool scoreIsLessThanXPercent(int percent) const
        {
            // remember that score is per 1000
            return scoreIsLessThanXPermille(10 * percent);
        }

        constexpr bool scoreIsLessThanXPermille(int permillage) const
        {
            if (!haveScore())
                return false; // unknown score doesn't count

            // remember that score is per 1000
            return score() < permillage;
        }

        bool updateWithNewerStats(TrackStats const& maybeNewerStats)
        {
            if (maybeNewerStats.lastHistoryId() <= lastHistoryId())
                return false; // 'newer' stats not actually newer

            *this = maybeNewerStats;
            return true;
        }

        TrackStats& operator=(TrackStats const& other) = default;

        bool operator==(TrackStats const& other) const
        {
            return _lastHistoryId == other._lastHistoryId
                && _lastHeard == other._lastHeard
                && _scoreHeardCount == other._scoreHeardCount
                && _averagePermillage == other._averagePermillage;
        }

        bool operator!=(TrackStats const& other) const
        {
            return !(*this == other);
        }

    private:
        TrackStats(uint lastHistoryId, QDateTime lastHeard,
                   quint32 scoreHeardCount, qint16 averagePermillage,
                   qint16 score)
         : _lastHistoryId(lastHistoryId),
           _lastHeard(lastHeard),
           _scoreHeardCount(scoreHeardCount),
           _averagePermillage(averagePermillage),
           _score(score)
        {
            //
        }

        static qint16 calculateScore(int scorePermillage, quint32 scoreHeardCount)
        {
            if (scorePermillage < 0 || scoreHeardCount < 3) return -1;

            if (scorePermillage > 1000)
                scorePermillage = 1000;

            if (scoreHeardCount >= 100)
                return scorePermillage;

            auto permillage = static_cast<quint16>(scorePermillage);

            return (permillage * scoreHeardCount + 500) / (scoreHeardCount + 1);
        }

        uint _lastHistoryId;
        QDateTime _lastHeard;
        quint32 _scoreHeardCount;
        qint16 _averagePermillage;
        qint16 _score;
    };
}
#endif
