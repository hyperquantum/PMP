/*
    Copyright (C) 2014-2018, Kevin Andre <hyperquantum@gmail.com>

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

#include "common/util.h"

#include "database.h"
#include "history.h"
#include "queue.h"
#include "queueentry.h"
#include "resolver.h"

#include <QtDebug>
#include <QTimer>

#include <algorithm>

namespace PMP {

    /* ========================== Candidate ========================== */

    class Generator::Candidate {
    public:
        Candidate(const FileHash& hashID, quint16 randomPermillageNumber)
         : _hash(hashID), _lengthMilliseconds(0),
           _randomPermillageNumber(randomPermillageNumber)
        {
            //
        }

        const FileHash& hash() const { return _hash; }

        void setLengthMilliseconds(uint milliseconds) {
            _lengthMilliseconds = milliseconds;
        }

        uint lengthMilliseconds() const { return _lengthMilliseconds; }

        quint16 randomPermillageNumber() const { return _randomPermillageNumber; }

    private:
        FileHash _hash;
        uint _lengthMilliseconds;
        quint16 _randomPermillageNumber;
    };

    /* ========================== Generator ========================== */

    Generator::Generator(Queue* queue, Resolver* resolver, History* history)
     : _randomEngine(Util::getRandomSeed()), _currentTrack(nullptr),
       _queue(queue), _resolver(resolver), _history(history),
       _enabled(false), _refillPending(false),
       _upcomingRuntimeSeconds(0), _upcomingTimer(new QTimer(this)),
       _noRepetitionSpan(60 * 60 /* one hour */), _userPlayingFor(0)
    {
        connect(
            _queue, &Queue::entryRemoved,
            this, &Generator::queueEntryRemoved
        );
        connect(
            _upcomingTimer, &QTimer::timeout,
            this, &Generator::checkRefillUpcomingBuffer
        );
        connect(
            _resolver, &Resolver::hashBecameAvailable,
            this, &Generator::hashBecameAvailable
        );
        connect(
            _resolver, &Resolver::hashBecameUnavailable,
            this, &Generator::hashBecameUnavailable
        );

        _hashesSource = _resolver->getAllHashes();
        _hashesInSource = _hashesSource.toSet();
        std::shuffle(_hashesSource.begin(), _hashesSource.end(), _randomEngine);
        qDebug() << "Generator: starting with source of size" << _hashesSource.size();
    }

    bool Generator::enabled() const {
        return _enabled;
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
            qDebug() << "Generator: no queue expansion because upcoming buffer is low";
            return;
        }

        expandQueue(expandCount, 15);
    }

    void Generator::currentTrackChanged(QueueEntry const* newTrack) {
        _currentTrack = newTrack;
    }

    void Generator::setUserPlayingFor(quint32 user) {
        _userPlayingFor = user;
    }

    void Generator::queueEntryRemoved(quint32, quint32) {
        requestQueueRefill();
    }

    void Generator::hashBecameAvailable(FileHash hash) {
        if (_hashesInSource.contains(hash)
            || _hashesSpent.contains(hash))
        {
            return; /* already known */
        }

        _hashesInSource.insert(hash);

        /* put it at a random index in the source list */
        int endIndex = _hashesSource.size();
        std::uniform_int_distribution<int> range(0, endIndex);
        int randomIndex = range(_randomEngine);

        /* Avoid shifting the list with 'insert()'; we append and then use swap.
           We can do this because the list is in random order anyway. */
        _hashesSource.append(hash); /* append is at endIndex */
        if (randomIndex < endIndex) {
            std::swap(_hashesSource[randomIndex], _hashesSource[endIndex]);
        }
    }

    void Generator::hashBecameUnavailable(PMP::FileHash hash) {
        /* We don't remove it from the source because that is expensive. We will simply
           leave it inside the source, because unavailable tracks are filtered when tracks
           are taken from the source list.

           We don't remove it from the spent list, because doing that could make the track
           reappear in the source too soon if it becomes available again. */
        (void)hash;
    }

    void Generator::requestQueueRefill() {
        if (_refillPending) return;
        _refillPending = true;
        QTimer::singleShot(100, this, SLOT(checkAndRefillQueue()));
    }

    quint16 Generator::getRandomPermillage() {
        std::uniform_int_distribution<int> range(0, 1000);
        return quint16(range(_randomEngine));
    }

    FileHash Generator::getNextRandomHash() {
        if (_hashesSource.isEmpty()) {
            if (_hashesSpent.isEmpty()) { return FileHash(); /* nothing available */ }

            /* rebuild source */
            qDebug() << "Generator: rebuilding source list";
            _hashesSource = _hashesSpent.toList();
            std::shuffle(_hashesSource.begin(), _hashesSource.end(), _randomEngine);
        }

        FileHash randomHash = _hashesSource.takeLast();
        if (!randomHash.empty()) { _hashesSpent.insert(randomHash); }

        auto sourceHashCount = _hashesSource.size();
        if (sourceHashCount % 10 == 0) {
            qDebug() << "Generator: source list down to" << sourceHashCount
                     << " ; spent list count:" << _hashesSpent.size();
        }

        if (sourceHashCount >= 11) {
            /* fetch some user stats in advance, for the next calls to this function */
            auto index = qBound(0, sourceHashCount - 11, sourceHashCount);
            uint id = _resolver->getID(_hashesSource[index]);
            _history->fetchMissingUserStats(id, _userPlayingFor);
        }

        return randomHash;
    }

    void Generator::checkRefillUpcomingBuffer() {
        int iterationsLeft = 8;
        while (iterationsLeft > 0
               && (_upcoming.length() < maximalUpcomingCount
                    || _upcomingRuntimeSeconds < desiredUpcomingRuntimeSeconds))
        {
            iterationsLeft--;

            FileHash randomHash = getNextRandomHash();
            if (randomHash.empty()) { break; /* nothing available */ }

            Candidate* c = new Candidate(randomHash, getRandomPermillage());

            if (satisfiesFilters(c, false)) {
                _upcoming.enqueue(c);
                _upcomingRuntimeSeconds += c->lengthMilliseconds() / 1000;
            }
            else {
                delete c;
            }

            /* urgent queue refill needed? */
            if (iterationsLeft >= 6
                && _enabled
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
                 << "; runtime:" << (_upcomingRuntimeSeconds / 60) << "min"
                 << (_upcomingRuntimeSeconds % 60) << "sec";

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
            _upcomingRuntimeSeconds -= c->lengthMilliseconds() / 1000;

            /* check filters again */
            bool ok = satisfiesFilters(c, true);

            /* check occurrence in queue */
            int nonRepetitionSpan = 0;
            if (ok) {
                if (_queue->checkPotentialRepetitionByAdd(c->hash(), _noRepetitionSpan,
                                                          &nonRepetitionSpan))
                {
                    ok = false;
                }
            }

            /* check occurrence in 'now playing' */
            if (ok && nonRepetitionSpan < _noRepetitionSpan) {
                QueueEntry const* current = _currentTrack;
                if (current != 0) {
                    FileHash const* currentHash = current->hash();
                    if (currentHash != 0 && c->hash() == *currentHash) {
                        ok = false;
                    }
                }
            }

            /* check last play time, taking the future queue position into account */
            if (ok) {
                QDateTime maxLastPlay =
                    QDateTime::currentDateTimeUtc().addSecs(nonRepetitionSpan)
                                                   .addSecs(-_noRepetitionSpan);

                QDateTime lastPlay = _history->lastPlayed(c->hash());

                if (lastPlay.isValid() && lastPlay > maxLastPlay)
                {
                    ok = false;
                }
                else {
                    uint id = _resolver->getID(c->hash());
                    auto userStats = _history->getUserStats(id, _userPlayingFor);
                    if (!userStats) { ok = false; }
                    else {
                        lastPlay = userStats->lastHeard;
                        if (lastPlay.isValid() && lastPlay > maxLastPlay) { ok = false; }
                    }
                }
            }

            if (ok) {
                _queue->enqueue(c->hash());
                tracksToGenerate--;
            }

            delete c;
        }

        if ((uint)_upcoming.length() < maximalUpcomingCount
            && !_upcomingTimer->isActive())
        {
            _upcomingTimer->start(upcomingTimerFreqMs);
        }

        /* return how many were added to the queue */
        return howManyTracksToAdd - tracksToGenerate;
    }

    bool Generator::satisfiesFilters(Candidate* candidate, bool strict) {
        /* do we have the hash? */
        const FileHash& hash = candidate->hash();
        if (hash.empty()) return false;

        /* can we find a file for the track? */
        if (!_resolver->haveFileFor(hash)) return false;

        /* get audio info */
        const AudioData& audioData = _resolver->findAudioData(hash);
        int msLength = audioData.trackLengthMilliseconds();

        /* is it a real track, not a short sound file? */
        if (msLength < 15000 && msLength >= 0) return false;

        /* is score within tolerance? */
        uint id = _resolver->getID(candidate->hash());
        auto userStats = _history->getUserStats(id, _userPlayingFor);
        if (!userStats) {
            if (strict) {
                qDebug() << "Generator: rejecting candidate" << id
                        << "because score is still unknown";
                return false;
            }
        }
        else {
            auto score = userStats->score;
            if (score >= 0 && score < candidate->randomPermillageNumber() - 100) {
                qDebug() << "Generator: rejecting candidate" << id
                        << "because it has score" << score << " (threshhold="
                        << (candidate->randomPermillageNumber() - 100) << ")";
                return false;
            }
        }

        /* save length if known */
        if (msLength >= 0)
            candidate->setLengthMilliseconds(msLength);

        return true;
    }

}
