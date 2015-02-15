/*
    Copyright (C) 2014-2015, Kevin Andre <hyperquantum@gmail.com>

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

#include "history.h"
#include "queue.h"
#include "queueentry.h"
#include "resolver.h"

#include <QtDebug>
#include <QTimer>

namespace PMP {

    class Generator::Candidate {
    public:
        Candidate(const HashID& hashID)
         : _hash(hashID), _lengthSeconds(0)
        {
            //
        }

        const HashID& hash() const { return _hash; }

        void setLengthSeconds(uint seconds) { _lengthSeconds = seconds; }
        uint lengthSeconds() const { return _lengthSeconds; }

    private:
        HashID _hash;
        uint _lengthSeconds;
    };

    Generator::Generator(Queue* queue, Resolver* resolver, History* history)
     : _currentTrack(0), _queue(queue), _resolver(resolver), _history(history),
       _enabled(false), _refillPending(false),
       _upcomingRuntimeSeconds(0), _upcomingTimer(new QTimer(this)),
       _noRepetitionSpan(60 * 60 /* one hour */)
    {
        connect(
            _queue, SIGNAL(entryRemoved(quint32, quint32)),
            this, SLOT(queueEntryRemoved(quint32, quint32))
        );
        connect(
            _upcomingTimer, SIGNAL(timeout()),
            this, SLOT(checkRefillUpcomingBuffer())
        );

        /* one time, to get a minimal start amount of tracks */
        checkRefillUpcomingBuffer();
    }

    bool Generator::enabled() const {
        return _enabled;
    }

    int Generator::noRepetitionSpan() const {
        return _noRepetitionSpan;
    }

    void Generator::setNoRepetitionSpan(int seconds) {
        if (_noRepetitionSpan == seconds) return; /* no change */

        qDebug() << "changing no-repetition span from" << _noRepetitionSpan << "to" << seconds;
        _noRepetitionSpan = seconds;

        emit noRepetitionSpanChanged(seconds);
    }

    void Generator::enable() {
        if (_enabled) return; /* enabled already */

        qDebug() << "generator enabled";
        _enabled = true;

        emit enabledChanged(true);
        _upcomingTimer->start(5000);

        /* Start filling the upcoming buffer at once, and already fill the queue a bit if
           possible. */
        checkRefillUpcomingBuffer();
    }

    void Generator::disable() {
        if (!_enabled) return; /* disabled already */

        qDebug() << "generator disabled";
        _enabled = false;

        _upcomingTimer->stop();
        emit enabledChanged(false);
    }

    void Generator::requestQueueExpansion() {
        if ((uint)_upcoming.size() < minimalUpcomingCount) {
            qDebug() << "generator: not executing queue expansion because upcoming buffer is low";
            return;
        }

        expandQueue(expandCount, 15);
    }

    void Generator::currentTrackChanged(QueueEntry const* newTrack) {
        _currentTrack = newTrack;
    }

    void Generator::queueEntryRemoved(quint32, quint32) {
        requestQueueRefill();
    }

    void Generator::requestQueueRefill() {
        if (_refillPending) return;
        _refillPending = true;
        QTimer::singleShot(100, this, SLOT(checkAndRefillQueue()));
    }

    void Generator::checkRefillUpcomingBuffer() {
        //int upcomingToGenerate = desiredUpcomingLength - _upcoming.length();
        int iterationsLeft = 8;
        while (iterationsLeft > 0
               && ((uint)_upcoming.length() < maximalUpcomingCount
                    || _upcomingRuntimeSeconds < desiredUpcomingRuntimeSeconds))
        {
            iterationsLeft--;

            HashID randomHash = _resolver->getRandom();
            if (randomHash.empty()) { break; /* nothing available */ }

            Candidate* c = new Candidate(randomHash);

            if (satisfiesFilters(c)) {
                _upcoming.enqueue(c);
                _upcomingRuntimeSeconds += c->lengthSeconds();
            }

            /* urgent queue refill needed? */
            if (iterationsLeft >= 6
                && (uint)_upcoming.length() >= minimalUpcomingCount
                && _queue->length() < desiredQueueLength)
            {
                requestQueueRefill();
                break;
            }
        }

        qDebug() << "generator: buffer length:" << _upcoming.length()
                 << "; runtime:" << (_upcomingRuntimeSeconds / 60) << "min"
                 << (_upcomingRuntimeSeconds % 60) << "sec";
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
            _upcomingRuntimeSeconds -= c->lengthSeconds();

            /* check filters again */
            bool ok = satisfiesFilters(c);

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
                    HashID const* currentHash = current->hash();
                    if (currentHash != 0 && c->hash() == *currentHash) {
                        ok = false;
                    }
                }
            }

            /* check last play time, taking the future queue position into account */
            if (ok) {
                QDateTime lastPlay = _history->lastPlayed(c->hash());
                QDateTime maxLastPlay =
                    QDateTime::currentDateTimeUtc().addSecs(nonRepetitionSpan)
                                                   .addSecs(-_noRepetitionSpan);

                if (lastPlay.isValid() && lastPlay > maxLastPlay)
                {
                    ok = false;
                }
            }

            if (ok) {
                _queue->enqueue(c->hash());
                tracksToGenerate--;
            }

            delete c;
        }

        /* return how many were added to the queue */
        return howManyTracksToAdd - tracksToGenerate;
    }

    bool Generator::satisfiesFilters(Candidate* candidate) {
        const HashID& hash = candidate->hash();

        /* can we find a file for the track? */
        if (!_resolver->haveAnyPathInfo(hash)) return false;

        /* get audio info */
        const AudioData& audioData = _resolver->findAudioData(hash);
        int trackLengthSeconds = audioData.trackLength();

        /* is it a real track, not a short sound file? */
        if (trackLengthSeconds < 15 && trackLengthSeconds >= 0) return false;

        candidate->setLengthSeconds(trackLengthSeconds);

        return true;
    }

}
