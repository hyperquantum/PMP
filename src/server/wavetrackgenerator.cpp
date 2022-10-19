/*
    Copyright (C) 2020-2022, Kevin Andre <hyperquantum@gmail.com>

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

namespace PMP
{
    namespace
    {
        static const int selectionFilterTakeCount = 22;
        static const int selectionFilterKeepCount = 10;
        static const int generationCountGoal = selectionFilterKeepCount * 2;
    }

    WaveTrackGenerator::WaveTrackGenerator(QObject* parent, RandomTracksSource* source,
                                           Resolver* resolver, History* history,
                                           TrackRepetitionChecker* repetitionChecker)
     : TrackGeneratorBase(parent, source, resolver, history, repetitionChecker),
       _trackGenerationFailCount(0),
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

            if (trackIsSuitable)
                tracks.append(track->hash());
        }

        _tracksDeliveredCount += tracks.size();

        calculateProgressAndEmitSignal();

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
        _trackGenerationFailCount = 0;
        _trackGenerationProgress = 0;
        _tracksDeliveredCount = 0;
        _upcoming.reserve(generationCountGoal);
        _buffer.reserve(selectionFilterKeepCount);
        QTimer::singleShot(0, this, &WaveTrackGenerator::upcomingRefillTimerAction);

        Q_EMIT waveStarted();
    }

    void WaveTrackGenerator::terminateWave()
    {
        if (!_waveActive)
            return;

        qDebug() << "terminating wave";

        _waveActive = false;

        /* all tracks will be marked as used and put back in the source */
        _upcoming.clear();
        _buffer.clear();

        Q_EMIT waveEnded(false);
    }

    void WaveTrackGenerator::upcomingRefillTimerAction()
    {
        if (!_waveActive)
            return;

        if (_trackGenerationProgress < generationCountGoal)
        {
            growBuffer();

            if (!_waveActive)
                return; // early quit

            if (_buffer.size() >= selectionFilterTakeCount)
                applySelectionFilterToBufferAndAppendToUpcoming();
        }

        if (_trackGenerationProgress >= generationCountGoal)
        {
            qDebug() << "generation complete";
            _waveGenerationCompleted = true;

            /* emit a zero progress signal */
            calculateProgressAndEmitSignal();
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

        if (fromSourceCount > 0)
            return; // we found one or more tracks that satisfy the criteria

        if (_trackGenerationFailCount
                                     < totalTrackCountInSource() - tracksToTakeFromSource)
        {
            _trackGenerationFailCount += tracksToTakeFromSource;
            return; // count failures
        }

        qDebug() << "failed to gather enough tracks that satisfy the criteria; giving up";
        terminateWave();
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

        for (auto const& track : qAsConst(tracks))
        {
            _upcoming.append(track);
            _trackGenerationProgress++;
        }

        _buffer.clear();

        qDebug() << "generation progress is now" << _trackGenerationProgress;
    }

    void WaveTrackGenerator::calculateProgressAndEmitSignal()
    {
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

            Q_EMIT waveProgress(_tracksDeliveredCount, total);
        }
        else /* wave completed */
        {
            qDebug() << "wave is now complete; delivered" << _tracksDeliveredCount
                     << "tracks";

            _waveActive = false;

            /* Deliver a final progress update, this confirms that the wave really has
               finished and was not cancelled. */
            Q_EMIT waveProgress(_tracksDeliveredCount, _tracksDeliveredCount);
            Q_EMIT waveEnded(true);
        }
    }

    void WaveTrackGenerator::criteriaChanged()
    {
        if (!_waveActive)
            return;

        // less strict criteria may still allow us to succeed, reset the fail counter
        _trackGenerationFailCount = 0;

        // filter upcoming list and recalculate wave progress

        auto oldUpcomingSize = _upcoming.size();

        auto filter =
                [this](Candidate const& c)
                {
                    return satisfiesBasicFilter(c) && satisfiesNonRepetition(c);
                };

        applyFilterToQueue(_upcoming, filter, generationCountGoal);

        auto newUpcomingSize = _upcoming.size();

        qDebug() << "dynamic mode wave criteria changed; removed"
                 << (oldUpcomingSize - newUpcomingSize)
                 << "tracks from the upcoming list,"
                 << newUpcomingSize << "tracks are remaining";

        calculateProgressAndEmitSignal();
    }

    void WaveTrackGenerator::desiredUpcomingCountChanged()
    {
        // irrelevant
    }

    int WaveTrackGenerator::selectionFilterCompare(const Candidate& t1,
                                                   const Candidate& t2)
    {
        auto user = criteria().user();

        /* we know user stats ARE available because of the basic filter */
        auto userStats1 = history().getUserStats(t1.id(), user).value();
        auto userStats2 = history().getUserStats(t2.id(), user).value();

        int permillage1 = userStats1.getScoreOr(0);
        int permillage2 = userStats2.getScoreOr(0);

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
        auto maybeUserStats = history().getUserStats(id, criteria().user());
        if (maybeUserStats.isNull())
        {
            qDebug() << "rejecting candidate" << id
                     << "because we don't have its user data yet";
            return false;
        }

        auto userStats = maybeUserStats.value();

        /* reject candidates that do not have a score yet */
        if (!userStats.haveScore())
        {
            return false;
        }

        if (userStats.scoreIsLessThanXPercent(60))
        {
            /* candidate's score does not measure up to a reasonable minimum */
            return false;
        }

        return true;
    }

}
