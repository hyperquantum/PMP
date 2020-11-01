/*
    Copyright (C) 2014-2020, Kevin Andre <hyperquantum@gmail.com>

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

#include "generator.h"

#include "database.h"
#include "dynamictrackgenerator.h"
#include "history.h"
#include "playerqueue.h"
#include "randomtrackssource.h"
#include "resolver.h"
#include "trackrepetitionchecker.h"
#include "wavetrackgenerator.h"

#include <QtDebug>
#include <QTimer>

#include <algorithm>

namespace PMP {

    namespace {
        static const int defaultNoRepetitionSpanSeconds = 60 * 60 /* one hour */;

        static const int desiredQueueLength = 10;
        static const int expandCount = 5;
    }

    Generator::Generator(PlayerQueue* queue, Resolver* resolver, History* history)
     : _randomTracksSource(new RandomTracksSource(this, resolver)),
       _repetitionChecker(new TrackRepetitionChecker(this, queue, history)),
       _trackGenerator(new DynamicTrackGenerator(this, _randomTracksSource, resolver,
                                                 history, _repetitionChecker)),
       _waveTrackGenerator(new WaveTrackGenerator(this, _randomTracksSource, resolver,
                                                  history, _repetitionChecker)),
       _queue(queue),
       _resolver(resolver),
       _history(history),
       _enabled(false),
       _refillPending(false),
       _waveActive(false)
    {
        _criteria.setNoRepetitionSpanSeconds(defaultNoRepetitionSpanSeconds);
        _repetitionChecker->setNoRepetitionSpanSeconds(defaultNoRepetitionSpanSeconds);

        _trackGenerator->setCriteria(_criteria);
        _waveTrackGenerator->setCriteria(_criteria);

        connect(
            _randomTracksSource, &RandomTracksSource::upcomingTrackNotification,
            this, &Generator::upcomingTrackNotification
        );
        connect(
            _queue, &PlayerQueue::entryRemoved,
            this, &Generator::queueEntryRemoved
        );
        connect(
            _waveTrackGenerator, &WaveTrackGenerator::waveStarted,
            this,
            [this]() {
                _waveActive = true;
                Q_EMIT waveStarting(_criteria.user());
            }
        );
        connect(
            _waveTrackGenerator, &WaveTrackGenerator::waveEnded,
            this,
            [this]() {
                _waveActive = false;
                Q_EMIT waveFinished(_criteria.user());
            }
        );
    }

    bool Generator::enabled() const {
        return _enabled;
    }

    bool Generator::waveActive() const {
        return _waveActive;
    }

    quint32 Generator::userPlayingFor() const {
        return _criteria.user();
    }

    int Generator::noRepetitionSpanSeconds() const
    {
        return _criteria.noRepetitionSpanSeconds();
    }

    History& Generator::history() {
        return *_history;
    }

    void Generator::enable() {
        if (_enabled) return; /* enabled already */

        qDebug() << "Generator enabled";
        _enabled = true;

        Q_EMIT enabledChanged(true);

        setDesiredUpcomingCount();

        _trackGenerator->enable();

        checkQueueRefillNeeded();
    }

    void Generator::disable() {
        if (!_enabled) return; /* disabled already */

        qDebug() << "Generator disabled";
        _enabled = false;

        _trackGenerator->disable();
        _waveTrackGenerator->terminateWave();

        Q_EMIT enabledChanged(false);
    }

    void Generator::requestQueueExpansion()
    {
        int generatedCount = 0;

        if (_waveActive) {
            auto tracks = _waveTrackGenerator->getTracks(expandCount);
            generatedCount += tracks.size();

            for (auto const& track : tracks)
            {
                _queue->enqueue(track);
            }

            if (generatedCount >= expandCount)
                return;
        }

        if (!_waveActive)
        {
            auto tracks = _trackGenerator->getTracks(expandCount - generatedCount);
            generatedCount += tracks.size();

            for (auto const& track : tracks)
            {
                _queue->enqueue(track);
            }
        }

        if (generatedCount == expandCount) {
            qDebug() << "queue expansion successful: generated" << generatedCount
                     << "tracks";
        }
        else if (generatedCount == 0) {
            qWarning() << "queue expansion failed: nothing generated";
        }
        else if (generatedCount < expandCount) {
            qWarning() << "queue expansion failed: only generated" << generatedCount
                       << "tracks instead of" << expandCount;
        }
        else { // generatedCount > expandCount
            qWarning() << "queue expansion too big: generated" << generatedCount
                       << "tracks instead of" << expandCount;
        }
    }

    void Generator::startWave() {
        qDebug() << "Generator::startWave() called";

        if (_criteria.user() <= 0)
            return; // FAIL, need a user's scores for a wave

        _waveTrackGenerator->startWave();
    }

    void Generator::terminateWave()
    {
        qDebug() << "Generator::terminateWave() called";

        _waveTrackGenerator->terminateWave();
    }

    void Generator::currentTrackChanged(QueueEntry const* newTrack)
    {
        _repetitionChecker->currentTrackChanged(newTrack);
    }

    void Generator::setUserGeneratingFor(quint32 user) {
        if (_criteria.user() == user)
            return; /* no change */

        auto oldUser = _criteria.user();
        qDebug() << "changing user from" << oldUser << "to" << user;

        /* if a wave is active, terminate it because a wave is bound to a user */
        terminateWave();

        /* apply new user */
        _criteria.setUser(user);

        /* make sure to prefetch track stats for the new user */
        _randomTracksSource->resetUpcomingTrackNotifications();

        /* give us some time to fetch track stats for the new user */
        _trackGenerator->freezeTemporarily();

        _repetitionChecker->setUserGeneratingFor(user);
        _trackGenerator->setCriteria(_criteria);
        _waveTrackGenerator->setCriteria(_criteria);
    }

    void Generator::setNoRepetitionSpanSeconds(int seconds) {
        if (_criteria.noRepetitionSpanSeconds() == seconds)
            return; /* no change */

        auto oldNoRepetitionSpan = _criteria.noRepetitionSpanSeconds();
        _criteria.setNoRepetitionSpanSeconds(seconds);

        qDebug() << "Generator: changing no-repetition span from" << oldNoRepetitionSpan
                 << "to" << seconds;

        _repetitionChecker->setNoRepetitionSpanSeconds(seconds);
        _trackGenerator->setCriteria(_criteria);
        _waveTrackGenerator->setCriteria(_criteria);

        Q_EMIT noRepetitionSpanChanged(seconds);
    }

    void Generator::upcomingTrackNotification(FileHash hash)
    {
        /* fetch user stats for this track that will soon enter our picture */
        uint id = _resolver->getID(hash);
        _history->fetchMissingUserStats(id, _criteria.user());
    }

    void Generator::queueEntryRemoved(quint32, quint32)
    {
        checkQueueRefillNeeded();
    }

    void Generator::queueRefillTimerAction() {
        _refillPending = false;

        if (!_enabled) return;

        int tracksToGenerate = 0;
        uint queueLength = _queue->length();
        if (queueLength < desiredQueueLength) {
            tracksToGenerate = desiredQueueLength - queueLength;
        }

        auto tracks =
                _waveActive
                    ? _waveTrackGenerator->getTracks(tracksToGenerate)
                    : _trackGenerator->getTracks(tracksToGenerate);

        for (auto const& track : tracks) {
            _queue->enqueue(track);
        }

        checkQueueRefillNeeded();
    }

    void Generator::checkQueueRefillNeeded()
    {
        if (_refillPending)
            return;

        int tracksNeeded = desiredQueueLength - (int)_queue->length();
        if (tracksNeeded <= 0)
            return;

        _refillPending = true;
        QTimer::singleShot(100, this, &Generator::queueRefillTimerAction);
    }

    void Generator::setDesiredUpcomingCount()
    {
        int count = std::max(desiredQueueLength + expandCount, 3 * expandCount);

        _trackGenerator->setDesiredUpcomingCount(count);
        _waveTrackGenerator->setDesiredUpcomingCount(count);
    }

    /*
    void Generator::advanceWave() {
        if (_waveRising) {
            if (_minimumPermillageByWave < 700)
                _minimumPermillageByWave += 100; // add 10%
            else
                _minimumPermillageByWave += 50; // add 5%

            if (_minimumPermillageByWave >= 850)
                _waveRising = false; // start descending again

            qDebug() << "wave has risen to" << _minimumPermillageByWave;
            return;
        }

        if (_minimumPermillageByWave <= 0)
            return;

        // wave is descending

        if (!_waveActive) {
            _minimumPermillageByWave--; // subtract 0.1%

            if (_minimumPermillageByWave >= 10) {
                _minimumPermillageByWave -= 10; // subtract an additional 1%

                if (_minimumPermillageByWave >= 500)
                    _minimumPermillageByWave -= 20; // subtract an additional 2%
            }

            qDebug() << "non-active wave has descended to" << _minimumPermillageByWave;
            return;
        }

        if (_minimumPermillageByWave >= 800)
            _minimumPermillageByWave -= 10; // subtract 1%
        else if (_minimumPermillageByWave >= 700)
            _minimumPermillageByWave -= 50; // subtract 5%
        else if (_waveActive) {
            _minimumPermillageByWave -= 100; // subtract 10%

            qDebug() << "wave is now no longer considered active (to the outside world)";

            _waveActive = false;
            emit waveFinished(_userPlayingFor);
        }

        qDebug() << "wave has descended to" << _minimumPermillageByWave;
    }
    */

    /*
    bool Generator::satisfiesWaveFilter(Candidate* candidate) {
        if (_minimumPermillageByWave <= 0) return true;

        uint id = candidate->id();
        auto userStats = _history->getUserStats(id, _userPlayingFor);
        if (!userStats) return false; // score data not loaded --> reject

        auto score = userStats->score;

        if (score < 0) { // score not defined yet (play count too low)?
            // reject randomly
            if (candidate->randomPermillageNumber() < _minimumPermillageByWave
                    || candidate->randomPermillageNumber2() < _minimumPermillageByWave)
            {
                qDebug() << "rejecting candidate" << id
                         << "randomly because it does not have a score yet";
                return false;
            }
        }
        else if (score < _minimumPermillageByWave) {
            qDebug() << "rejecting candidate" << id << "because it has score" << score
                     << "while the wave currently requires at least"
                     << _minimumPermillageByWave;
            return false;
        }
        else if (_minimumPermillageByWave >= 500) {
            // check if score is too high
            auto extra = score - _minimumPermillageByWave;

            if (extra > 100 &&
                    (candidate->randomPermillageNumber() < extra
                    || candidate->randomPermillageNumber2() < extra))
            {
                qDebug() << "rejecting candidate" << id << "because its score (" << score
                         << ") is randomly too high for the current wave status ("
                         << _minimumPermillageByWave << ")";
                return false;
            }
        }

        return true;
    }
    */
}
