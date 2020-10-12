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

#include "common/audiodata.h"
#include "common/util.h"

#include "database.h"
#include "history.h"
#include "playerqueue.h"
#include "queueentry.h"
#include "randomtrackssource.h"
#include "resolver.h"

#include <QtDebug>
#include <QTimer>

#include <algorithm>

namespace PMP {

    /* ========================== Candidate ========================== */

    class Generator::Candidate {
    public:
        Candidate(uint id, FileHash const& hash, AudioData const& audioData,
                  quint16 randomPermillageNumber1, quint16 randomPermillageNumber2)
         : _id(id),
           _hash(hash),
           _audioData(audioData),
           _randomPermillageNumber1(randomPermillageNumber1),
           _randomPermillageNumber2(randomPermillageNumber2)
        {
            //
        }

        uint id() const { return _id; }
        const FileHash& hash() const { return _hash; }

        qint64 lengthMilliseconds() const { return _audioData.trackLengthMilliseconds(); }

        quint16 randomPermillageNumber() const { return _randomPermillageNumber1; }

        quint16 randomPermillageNumber2() const { return _randomPermillageNumber2; }

    private:
        uint _id;
        FileHash _hash;
        AudioData _audioData;
        quint16 _randomPermillageNumber1;
        quint16 _randomPermillageNumber2;
    };

    /* ========================== Generator ========================== */

    Generator::Generator(PlayerQueue* queue, Resolver* resolver, History* history)
     : _randomTracksSource(new RandomTracksSource(this, resolver)),
       _randomEngine(Util::getRandomSeed()),
       _currentTrack(nullptr),
       _queue(queue),
       _resolver(resolver),
       _history(history),
       _upcomingTimer(new QTimer(this)),
       _upcomingRuntimeMilliseconds(0),
       _noRepetitionSpan(60 * 60 /* one hour */), _minimumPermillageByWave(0),
       _userPlayingFor(0), _enabled(false), _refillPending(false), _waveActive(false),
       _waveRising(false)
    {
        connect(
            _randomTracksSource, &RandomTracksSource::upcomingTrackNotification,
            this, &Generator::upcomingTrackNotification
        );
        connect(
            _queue, &PlayerQueue::entryRemoved,
            this, &Generator::queueEntryRemoved
        );
        connect(
            _upcomingTimer, &QTimer::timeout,
            this, &Generator::checkRefillUpcomingBuffer
        );
    }

    bool Generator::enabled() const {
        return _enabled;
    }

    bool Generator::waveActive() const {
        return _waveActive;
    }

    quint32 Generator::userPlayingFor() const {
        return _userPlayingFor;
    }

    int Generator::noRepetitionSpan() const {
        return _noRepetitionSpan;
    }

    History& Generator::history() {
        return *_history;
    }

    void Generator::setNoRepetitionSpan(int seconds) {
        if (_noRepetitionSpan == seconds) return; /* no change */

        qDebug() << "Generator: changing no-repetition span from" << _noRepetitionSpan
                 << "to" << seconds;
        _noRepetitionSpan = seconds;

        emit noRepetitionSpanChanged(seconds);
    }

    void Generator::startWave() {
        qDebug() << "Generator::startWave() called";

        if (_userPlayingFor <= 0)
            return; /* need a user's scores for that */

        bool starting = !_waveActive;

        _waveActive = true;
        _waveRising = true;

        if (_minimumPermillageByWave <= 500)
            _minimumPermillageByWave = 500;

        if (starting)
            emit waveStarting(_userPlayingFor);

        checkFirstUpcomingAgainAfterFiltersChanged();
    }

    void Generator::enable() {
        if (_enabled) return; /* enabled already */

        qDebug() << "Generator enabled";
        _enabled = true;

        emit enabledChanged(true);
        _upcomingTimer->start(upcomingTimerFreqMs);

        /* Start filling the upcoming buffer at once, and already fill the queue a bit if
           possible. */
        checkRefillUpcomingBuffer();
    }

    void Generator::disable() {
        if (!_enabled) return; /* disabled already */

        qDebug() << "Generator disabled";
        _enabled = false;

        _upcomingTimer->stop();
        emit enabledChanged(false);
    }

    void Generator::requestQueueExpansion() {
        if (_upcoming.size() < minimalUpcomingCount) {
            qWarning() << "Generator: no queue expansion because upcoming buffer is low";
            return;
        }

        expandQueue(expandCount, 4 * expandCount);
    }

    void Generator::currentTrackChanged(QueueEntry const* newTrack) {
        _currentTrack = newTrack;
    }

    void Generator::setUserPlayingFor(quint32 user) {
        if (_userPlayingFor == user) return; /* no change */

        auto oldUser = _userPlayingFor;
        _userPlayingFor = user;

        qDebug() << "changing user from" << oldUser << "to" << user;

        /* make sure to prefetch track stats for the new user */
        _randomTracksSource->resetUpcomingTrackNotifications();

        /* if a wave is active, terminate it */
        if (_waveActive) {
            _waveActive = false;
            _waveRising = false;
            _minimumPermillageByWave = 0;
            emit waveFinished(oldUser);
        }

        checkFirstUpcomingAgainAfterFiltersChanged();
    }

    void Generator::upcomingTrackNotification(FileHash hash)
    {
        /* fetch user stats for this track that will soon enter our picture */
        uint id = _resolver->getID(hash);
        _history->fetchMissingUserStats(id, _userPlayingFor);
    }

    void Generator::queueEntryRemoved(quint32, quint32) {
        requestQueueRefill();
    }

    void Generator::requestQueueRefill() {
        if (_refillPending) return;
        _refillPending = true;
        QTimer::singleShot(100, this, &Generator::checkAndRefillQueue);
    }

    quint16 Generator::getRandomPermillage() {
        std::uniform_int_distribution<int> range(0, 1000);
        return quint16(range(_randomEngine));
    }

    FileHash Generator::getNextRandomHash()
    {
        auto hash = _randomTracksSource->takeTrack();

        return hash;
    }

    Generator::Candidate* Generator::createCandidate(const FileHash& hash)
    {
        if (hash.isNull()) {
            qWarning() << "the null hash turned up as a potential candidate";
            return nullptr;
        }

        if (!_resolver->haveFileFor(hash)) {
            qDebug() << "cannot use hash" << hash
                     << "as a candidate because we don't have a file for it";
            return nullptr;
        }

        uint id = _resolver->getID(hash);
        if (id <= 0) {
            qDebug() << "cannot use hash" << hash
                     << "as a candidate because it hasn't been registered";
            return nullptr;
        }

        const AudioData& audioData = _resolver->findAudioData(hash);

        return new Candidate(id, hash, audioData,
                             getRandomPermillage(), getRandomPermillage());
    }

    void Generator::checkRefillUpcomingBuffer() {
        int iterationsLeft = 8;
        while (iterationsLeft > 0
               && (_upcoming.length() < maximalUpcomingCount
                    || _upcomingRuntimeMilliseconds < desiredUpcomingRuntimeMilliseconds))
        {
            iterationsLeft--;

            FileHash randomHash = getNextRandomHash();
            if (randomHash.isNull()) { break; /* nothing available */ }

            auto c = createCandidate(randomHash);
            if (!c) continue; /* could not create a valid candidate */

            if (satisfiesBasicFilter(c, false)) {
                _upcoming.enqueue(c);
                if (c->lengthMilliseconds() > 0)
                    _upcomingRuntimeMilliseconds += c->lengthMilliseconds();
            }
            else {
                delete c;
            }

            /* urgent queue refill needed? */
            if (iterationsLeft >= 6 && _enabled
                && _upcoming.length() >= minimalUpcomingCount
                && _queue->length() < desiredQueueLength)
            {
                /* bail out early */
                break;
            }
        }

        /* can we do a queue refill right away? */
        if (_enabled && iterationsLeft >= 6
            && _upcoming.length() >= minimalUpcomingCount
            && _queue->length() < desiredQueueLength)
        {
            requestQueueRefill();
        }

        qDebug() << "Generator: buffer length:" << _upcoming.length()
                 << "; runtime:"
                 << Util::millisecondsToLongDisplayTimeText(_upcomingRuntimeMilliseconds);

        if (_upcoming.length() >= maximalUpcomingCount
            && !(_enabled && _queue->length() < desiredQueueLength))
        {
            _upcomingTimer->stop();
        }
    }

    void Generator::checkAndRefillQueue() {
        _refillPending = false;

        if (!_enabled) return;

        int tracksToGenerate = 0;
        uint queueLength = _queue->length();
        if (queueLength < desiredQueueLength) {
            tracksToGenerate = desiredQueueLength - queueLength;
        }

        expandQueue(tracksToGenerate, 15);
    }

    int Generator::expandQueue(int howManyTracksToAdd, int maxIterations) {
        int iterationsLeft = maxIterations;
        int tracksToGenerate = howManyTracksToAdd;

        while (iterationsLeft > 0
                && tracksToGenerate > 0
                && !_upcoming.empty())
        {
            iterationsLeft--;

            Candidate* c = _upcoming.dequeue();
            if (c->lengthMilliseconds() > 0)
                _upcomingRuntimeMilliseconds -= c->lengthMilliseconds();

            /* check filters again */
            bool ok =
                satisfiesBasicFilter(c, true)
                    && (_waveActive ? satisfiesWaveFilter(c) : satisfiesNonWaveFilter(c))
                    && satisfiesNonRepetition(c);

            if (ok) {
                _queue->enqueue(c->hash());
                tracksToGenerate--;

                advanceWave();
            }

            /* the track is marked as 'used' even when it has been deemed unsuitable, so
               that it won't come back again for a long time */
            _randomTracksSource->putBackUsedTrack(c->hash());

            delete c;
        }

        if (uint(_upcoming.length()) < maximalUpcomingCount
            && !_upcomingTimer->isActive())
        {
            _upcomingTimer->start(upcomingTimerFreqMs);
        }

        /* return how many were added to the queue */
        return howManyTracksToAdd - tracksToGenerate;
    }

    void Generator::checkFirstUpcomingAgainAfterFiltersChanged() {
        if (_upcoming.empty()) {
            if (!_upcomingTimer->isActive())
                _upcomingTimer->start(upcomingTimerFreqMs);

            return;
        }

        auto firstUpcoming = _upcoming.head();

        if (satisfiesBasicFilter(firstUpcoming, true)
            && (_waveActive
                ? satisfiesWaveFilter(firstUpcoming)
                : satisfiesNonWaveFilter(firstUpcoming)))
        {
            return; /* OK */
        }

        qDebug() << "removing first track from buffer because it does not pass the"
                 << "filters anymore after a filter change";

        auto candidate = _upcoming.dequeue();
        if (candidate->lengthMilliseconds() > 0)
            _upcomingRuntimeMilliseconds -= candidate->lengthMilliseconds();
        delete candidate;

        if (!_upcomingTimer->isActive())
            _upcomingTimer->start(upcomingTimerFreqMs);
    }

    void Generator::advanceWave() {
        if (_waveRising) {
            if (_minimumPermillageByWave < 700)
                _minimumPermillageByWave += 100; /* add 10% */
            else
                _minimumPermillageByWave += 50; /* add 5% */

            if (_minimumPermillageByWave >= 850)
                _waveRising = false; /* start descending again */

            qDebug() << "wave has risen to" << _minimumPermillageByWave;
            return;
        }

        if (_minimumPermillageByWave <= 0)
            return;

        /* wave is descending */

        if (!_waveActive) {
            _minimumPermillageByWave--; /* subtract 0.1% */

            if (_minimumPermillageByWave >= 10) {
                _minimumPermillageByWave -= 10; /* subtract an additional 1% */

                if (_minimumPermillageByWave >= 500)
                    _minimumPermillageByWave -= 20; /* subtract an additional 2% */
            }

            qDebug() << "non-active wave has descended to" << _minimumPermillageByWave;
            return;
        }

        if (_minimumPermillageByWave >= 800)
            _minimumPermillageByWave -= 10; /* subtract 1% */
        else if (_minimumPermillageByWave >= 700)
            _minimumPermillageByWave -= 50; /* subtract 5% */
        else if (_waveActive) {
            _minimumPermillageByWave -= 100; /* subtract 10% */

            qDebug() << "wave is now no longer considered active (to the outside world)";

            _waveActive = false;
            emit waveFinished(_userPlayingFor);
        }

        qDebug() << "wave has descended to" << _minimumPermillageByWave;
    }

    bool Generator::satisfiesBasicFilter(Candidate* candidate, bool strict)
    {
        /* is it a real track, not a short sound file? */
        auto lengthMilliseconds = candidate->lengthMilliseconds();
        if (lengthMilliseconds < 15000 && lengthMilliseconds >= 0)
            return false;

        /* are track stats available? */
        uint id = candidate->id();
        auto userStats = _history->getUserStats(id, _userPlayingFor);
        if (!userStats) {
            if (strict) {
                qDebug() << "rejecting candidate" << id
                         << "because we don't have its user data yet";
                return false;
            }
        }

        return true;
    }

    bool Generator::satisfiesNonWaveFilter(Candidate* candidate)
    {
        if (_waveActive)
            return true;

        /* is score within tolerance? */
        uint id = candidate->id();
        auto userStats = _history->getUserStats(id, _userPlayingFor);

        if (!userStats)
            return false;

        auto score = userStats->score;
        if (score >= 0 && score < candidate->randomPermillageNumber() - 100) {
            qDebug() << "rejecting candidate" << id
                     << "because it has score" << score << " (threshhold="
                     << (candidate->randomPermillageNumber() - 100) << ")";
            return false;
        }

        return true;
    }

    bool Generator::satisfiesWaveFilter(Candidate* candidate) {
        if (_minimumPermillageByWave <= 0) return true;

        uint id = candidate->id();
        auto userStats = _history->getUserStats(id, _userPlayingFor);
        if (!userStats) return false; /* score data not loaded --> reject */

        auto score = userStats->score;

        if (score < 0) { /* score not defined yet (play count too low)? */
            /* reject randomly */
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
            /* check if score is too high */
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

    bool Generator::satisfiesNonRepetition(Candidate* candidate)
    {
        FileHash const& hash = candidate->hash();

        /* check occurrence in queue */

        auto repetition = _queue->checkPotentialRepetitionByAdd(hash, _noRepetitionSpan);

        if (repetition.isRepetition())
        {
            return false;
        }

        qint64 millisecondsCounted = repetition.millisecondsCounted();
        if (millisecondsCounted >= _noRepetitionSpan * qint64(1000)) {
            return true;
        }

        /* check occurrence in 'now playing' */
        QueueEntry const* trackNowPlaying = _currentTrack;
        if (trackNowPlaying) {
            FileHash const* currentHash = trackNowPlaying->hash();
            if (currentHash && hash == *currentHash) {
                return false;
            }
        }

        /* check last play time, taking the future queue position into account */

        QDateTime maxLastPlay =
                QDateTime::currentDateTimeUtc().addMSecs(millisecondsCounted)
                                               .addSecs(-_noRepetitionSpan);

        QDateTime lastPlay = _history->lastPlayed(hash);
        if (lastPlay.isValid() && lastPlay > maxLastPlay)
        {
            return false;
        }

        auto userStats = _history->getUserStats(candidate->id(), _userPlayingFor);
        if (!userStats) {
            return false;
        }

        lastPlay = userStats->lastHeard;
        if (lastPlay.isValid() && lastPlay > maxLastPlay) {
            return false;
        }

        return true;
    }

}
