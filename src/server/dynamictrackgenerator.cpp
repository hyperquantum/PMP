/*
    Copyright (C) 2014-2021, Kevin Andre <hyperquantum@gmail.com>

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

#include "dynamictrackgenerator.h"

#include "common/audiodata.h"
#include "common/util.h"

#include "history.h"
#include "randomtrackssource.h"

#include <QDateTime>
#include <QTimer>

namespace PMP
{
    namespace
    {
        static const int selectionFilterTakeCount = 12;
        static const int selectionFilterKeepCount = 6;
    }

    DynamicTrackGenerator::DynamicTrackGenerator(QObject* parent,
                                                 RandomTracksSource* source,
                                                 Resolver* resolver, History* history,
                                                TrackRepetitionChecker* repetitionChecker)
     : TrackGeneratorBase(parent, source, resolver, history, repetitionChecker),
       _enabled(false),
       _refillPending(false),
       _temporaryFreeze(false)
    {
        //
    }

    QVector<FileHash> DynamicTrackGenerator::getTracks(int count)
    {
        QVector<FileHash> tracks;
        tracks.reserve(count);

        while (tracks.size() < count && !_upcoming.isEmpty())
        {
            auto track = _upcoming.dequeue();

            bool trackIsSuitable =
                    satisfiesFilters(*track) && satisfiesNonRepetition(*track);

            if (trackIsSuitable)
                tracks.append(track->hash());
        }

        checkIfRefillNeeded();

        qDebug() << "returning" << tracks.size() << "tracks";
        return tracks;
    }

    void DynamicTrackGenerator::enable()
    {
        if (_enabled) return;

        _enabled = true;

        checkIfRefillNeeded();
    }

    void DynamicTrackGenerator::disable()
    {
        if (!_enabled) return;

        _enabled = false;
    }

    void DynamicTrackGenerator::freezeTemporarily()
    {
        if (_temporaryFreeze)
            return; // already frozen

        qDebug() << "track generator freezing";
        _temporaryFreeze = true;

        QTimer::singleShot(
            250, this,
            [this]()
            {
                qDebug() << "track generator no longer frozen";
                _temporaryFreeze = false;
                checkIfRefillNeeded();
            }
        );
    }

    void DynamicTrackGenerator::upcomingRefillTimerAction()
    {
        _refillPending = false;

        if (_temporaryFreeze)
            return; // we'll be back later

        int attempts = 3;
        int added = 0;

        while (attempts > 0 && _upcoming.size() < desiredUpcomingCount())
        {
            attempts--;

            auto tracks =
                    takeFromSourceAndApplyBasicFilter(selectionFilterTakeCount,
                                                      selectionFilterTakeCount * 2,
                                                      true);

            if (tracks.size() == 0)
                continue; // have to try again

            tracks =
                    applySelectionFilter(tracks, selectionFilterKeepCount,
                                         [this](auto& a, auto& b) {
                                             return selectionFilterCompare(a, b);
                                         });
            for (auto const& track : qAsConst(tracks))
            {
                _upcoming.append(track);
                added++;
            }
        }

        qDebug() << "upcoming track list: count=" << _upcoming.size()
                 << "; added=" << added;

        /* maybe we're not done yet */
        checkIfRefillNeeded();
    }

    void DynamicTrackGenerator::criteriaChanged()
    {
        auto oldUpcomingSize = _upcoming.size();

        applyBasicFilterToQueue(_upcoming, desiredUpcomingCount());

        auto newUpcomingSize = _upcoming.size();

        qDebug() << "dynamic mode criteria changed; removed"
                 << (oldUpcomingSize - newUpcomingSize)
                 << "tracks from the upcoming list,"
                 << newUpcomingSize << "tracks are remaining";

        checkIfRefillNeeded();
    }

    void DynamicTrackGenerator::desiredUpcomingCountChanged()
    {
        _upcoming.reserve(desiredUpcomingCount());

        checkIfRefillNeeded();
    }

    void DynamicTrackGenerator::checkIfRefillNeeded()
    {
        if (_refillPending)
            return;

        if (desiredUpcomingCount() <= _upcoming.size())
            return;

        _refillPending = true;
        QTimer::singleShot(40, this, &DynamicTrackGenerator::upcomingRefillTimerAction);
    }

    int DynamicTrackGenerator::selectionFilterCompare(const Candidate& t1,
                                                      const Candidate& t2)
    {
        auto user = criteria().user();
        auto userStats1 = history().getUserStats(t1.id(), user);
        auto userStats2 = history().getUserStats(t2.id(), user);

        if (!userStats1 || !userStats2)
        {
            if (userStats1)
                return 1; // 1 is better
            else if (userStats2)
                return -1; // 2 is better
            else
                return 0; // equally worse
        }

        int permillage1 =
                userStats1->haveScore() ? userStats1->score : t1.randomPermillageNumber();
        int permillage2 =
                userStats2->haveScore() ? userStats2->score : t2.randomPermillageNumber();

        if (permillage1 < permillage2) return -1; // 2 is better
        if (permillage1 > permillage2) return 1; // 1 is better

        QDateTime lastHeard1 = userStats1->lastHeard;
        QDateTime lastHeard2 = userStats2->lastHeard;

        if (lastHeard1.isValid() && lastHeard2.isValid())
        {
            if (lastHeard1 < lastHeard2) return 1; // 1 is better
            if (lastHeard1 > lastHeard2) return -1; // 2 is better
        }
        else if (lastHeard1.isValid())
        {
            return -1; // 2 is better
        }
        else if (lastHeard2.isValid())
        {
            return 1; // 1 is better
        }

        // fallback: compare ID's
        return Util::compare(t1.id(), t2.id());
    }

    bool DynamicTrackGenerator::satisfiesFilters(Candidate& candidate)
    {
        if (!satisfiesBasicFilter(candidate))
            return false;

        // is score within tolerance?
        uint id = candidate.id();
        auto userStats = history().getUserStats(id, criteria().user());

        if (!userStats)
            return false;

        auto score = userStats->score;
        auto scoreThreshold = candidate.randomPermillageNumber() - 100;
        if (score >= 0 && score < scoreThreshold)
        {
            qDebug() << "rejecting candidate" << id
                     << "because it has score" << score
                     << "(threshhold:" << scoreThreshold << ")";
            return false;
        }

        return true;
    }

    bool DynamicTrackGenerator::satisfiesBasicFilter(Candidate const& candidate)
    {
        // is it a real track, not a short sound file?
        if (candidate.lengthIsLessThanXSeconds(15))
            return false;

        // are track stats available?
        uint id = candidate.id();
        auto userStats = history().getUserStats(id, criteria().user());
        if (!userStats)
        {
            qDebug() << "rejecting candidate" << id
                     << "because we don't have its user data yet";
            return false;
        }

        if (userStats->scoreLessThanXPercent(30))
        {
            /* reject candidates with a very low score */
            return false;
        }

        return true;
    }

}
