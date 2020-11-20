/*
    Copyright (C) 2020, Kevin Andre <hyperquantum@gmail.com>

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

#include "wavetrackgenerator.h"

#include "history.h"
#include "randomtrackssource.h"

#include <QTimer>

namespace PMP {

    namespace {
        static const int selectionFilterTakeCount = 30;
        static const int selectionFilterKeepCount = 15;
        static const int generationCountGoal = selectionFilterKeepCount * 2;
    }

    WaveTrackGenerator::WaveTrackGenerator(QObject* parent, RandomTracksSource* source,
                                           Resolver* resolver, History* history,
                                           TrackRepetitionChecker* repetitionChecker)
     : TrackGeneratorBase(parent, source, resolver, history, repetitionChecker),
       _trackGenerationProgress(0),
       _tracksDeliveredCount(0),
       _waveActive(false),
       _waveGenerationCompleted(false)
    {
        //
    }

    QVector<FileHash> WaveTrackGenerator::getTracks(int count)
    {
        if (!_waveActive)
            return {};

        QVector<FileHash> tracks;
        tracks.reserve(count);

        while (tracks.size() < count && !_upcoming.isEmpty())
        {
            auto track = _upcoming.dequeue();

            bool trackIsSuitable =
                    satisfiesBasicFilter(*track) && satisfiesNonRepetition(*track);

            /* the track is marked as 'used' even when it has been deemed unsuitable, so
               that it won't come back again for a long time */
            source().putBackUsedTrack(track->hash());

            if (trackIsSuitable)
                tracks.append(track->hash());
        }

        _tracksDeliveredCount += tracks.size();

        if (!_waveGenerationCompleted) /* final size not known yet */
        {
            qDebug() << "wave progress: delivered" << _tracksDeliveredCount
                     << "of an unknown total";
        }
        else if (!_upcoming.isEmpty())
        {
            int total = _tracksDeliveredCount + _upcoming.size();
            qDebug() << "wave progress: delivered" << _tracksDeliveredCount
                     << "of" << total;
        }
        else /* wave completed */
        {
            qDebug() << "wave is now complete; delivered" << _tracksDeliveredCount
                     << "tracks";

            _waveActive = false;
            Q_EMIT waveEnded();
        }

        qDebug() << "delivering" << tracks.size() << "tracks now";
        return tracks;
    }

    void WaveTrackGenerator::startWave()
    {
        if (_waveActive)
            return;

        qDebug() << "starting wave";

        _waveActive = true;
        _waveGenerationCompleted = false;
        _trackGenerationProgress = 0;
        _tracksDeliveredCount = 0;
        QTimer::singleShot(0, this, &WaveTrackGenerator::upcomingRefillTimerAction);

        Q_EMIT waveStarted();
    }

    void WaveTrackGenerator::terminateWave()
    {
        if (!_waveActive)
            return;

        qDebug() << "terminating wave";

        _waveActive = false;

        /* put all tracks back in the source as "used", so they won't immediately
           return as tracks for the regular dynamic mode */
        for (auto track : _upcoming)
        {
            source().putBackUsedTrack(track->hash());
        }
        for (auto track : _buffer)
        {
            source().putBackUsedTrack(track->hash());
        }

        _upcoming.clear();
        _buffer.clear();

        Q_EMIT waveEnded();
    }

    void WaveTrackGenerator::upcomingRefillTimerAction()
    {
        if (!_waveActive)
            return;

        if (_trackGenerationProgress < generationCountGoal)
        {
            growBuffer();

            if (_buffer.size() >= selectionFilterTakeCount)
                applySelectionFilterToBufferAndAppendToUpcoming();
        }

        if (_trackGenerationProgress >= generationCountGoal)
        {
            qDebug() << "generation complete";
            _waveGenerationCompleted = true;
        }
        else
            QTimer::singleShot(40, this, &WaveTrackGenerator::upcomingRefillTimerAction);
    }

    void WaveTrackGenerator::growBuffer()
    {
        int tracksToTakeFromSource = selectionFilterTakeCount - _buffer.size();

        if (tracksToTakeFromSource <= 0)
            return;

        int upcomingDurationEstimate =
                _upcoming.size() * 3 * 60 * 1000; // estimate 3 minutes per track

        auto filter =
                [this, upcomingDurationEstimate](const Candidate& c)
                {
                    return satisfiesBasicFilter(c)
                        && satisfiesNonRepetition(c, upcomingDurationEstimate);
                };

        auto tracks =
                takeFromSourceAndApplyFilter(tracksToTakeFromSource,
                                             selectionFilterTakeCount,
                                             false, filter);

        auto fromSourceCount = tracks.size();

        _buffer.append(tracks);

        qDebug() << "tried to get" << tracksToTakeFromSource
                 << "tracks from source, got" << fromSourceCount
                 << "after filtering, buffer size is now" << _buffer.size();
    }

    void WaveTrackGenerator::applySelectionFilterToBufferAndAppendToUpcoming()
    {
        int oldBufferSize = _buffer.size();

        auto tracks =
                applySelectionFilter(_buffer, selectionFilterKeepCount,
                                     [this](auto& a, auto& b) {
                                         return selectionFilterCompare(a, b);
                                     });

        qDebug() << "applied selection filter to buffer; reduced size from"
                 << oldBufferSize << "to" << tracks.size();

        for (auto track : tracks) {
            _upcoming.append(track);
            _trackGenerationProgress++;
        }

        _buffer.clear();

        qDebug() << "generation progress is now" << _trackGenerationProgress;
    }

    void WaveTrackGenerator::criteriaChanged()
    {
        // TODO : filter upcoming list and recalculate wave progress



    }

    void WaveTrackGenerator::desiredUpcomingCountChanged()
    {
        // irrelevant
    }

    int WaveTrackGenerator::selectionFilterCompare(const Candidate& t1,
                                                   const Candidate& t2)
    {
        auto user = criteria().user();
        auto userStats1 = history().getUserStats(t1.id(), user);
        auto userStats2 = history().getUserStats(t2.id(), user);

        int permillage1 = userStats1->haveScore() ? userStats1->score : 0;
        int permillage2 = userStats2->haveScore() ? userStats2->score : 0;

        if (permillage1 < permillage2) return -1; // 2 is better
        if (permillage1 > permillage2) return 1; // 1 is better

        return 0; // we consider them equal
    }

    bool WaveTrackGenerator::satisfiesBasicFilter(Candidate const& candidate)
    {
        // is it a real track, not a short sound file?
        if (candidate.lengthIsLessThanXSeconds(30))
            return false;

        // are track stats available?
        uint id = candidate.id();
        auto userStats = history().getUserStats(id, criteria().user());
        if (!userStats) {
            qDebug() << "rejecting candidate" << id
                     << "because we don't have its user data yet";
            return false;
        }

        /* reject candidates that do not have a score yet */
        if (!userStats->haveScore()) {
            return false;
        }

        if (userStats->scoreLessThanXPercent(60)) {
            /* candidate's score does not measure up to a reasonable minimum */
            return false;
        }

        return true;
    }

}