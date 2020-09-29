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

#ifndef PMP_GENERATOR_H
#define PMP_GENERATOR_H

#include "common/filehash.h"

#include <QObject>
#include <QQueue>
#include <QSet>
#include <QTimer>

#include <random>

namespace PMP {

    class History;
    class PlayerQueue;
    class QueueEntry;
    class RandomTracksSource;
    class Resolver;

    class Generator : public QObject {
        Q_OBJECT
    public:
        Generator(PlayerQueue* queue, Resolver* resolver, History* history);

        bool enabled() const;
        bool waveActive() const;

        quint32 userPlayingFor() const;

        int noRepetitionSpan() const;

        History& history();

    public slots:
        void enable();
        void disable();
        void requestQueueExpansion();

        void setNoRepetitionSpan(int seconds);

        void startWave();

        void currentTrackChanged(QueueEntry const* newTrack);
        void setUserPlayingFor(quint32 user);

    Q_SIGNALS:
        void enabledChanged(bool enabled);
        void noRepetitionSpanChanged(int seconds);
        void waveStarting(quint32 user);
        void waveFinished(quint32 user);

    private Q_SLOTS:
        void upcomingTrackNotification(FileHash hash);
        void queueEntryRemoved(quint32, quint32);

        void checkRefillUpcomingBuffer();
        void checkAndRefillQueue();

    private:
        class Candidate;

        static const int upcomingTimerFreqMs = 5000;
        static const int desiredQueueLength = 10;
        static const int expandCount = 5;
        static const int minimalUpcomingCount = 2 * desiredQueueLength;
        static const int maximalUpcomingCount = 3 * desiredQueueLength + 3 * expandCount;
        static const int desiredUpcomingRuntimeSeconds = 3600; /* 1 hour */

        quint16 getRandomPermillage();
        FileHash getNextRandomHash();
        void checkFirstUpcomingAgainAfterFiltersChanged();
        void requestQueueRefill();
        int expandQueue(int howManyTracksToAdd, int maxIterations);
        void advanceWave();
        bool satisfiesFilters(Candidate* candidate, bool strict);
        bool satisfiesWaveFilter(Candidate* candidate);
        bool satisfiesNonRepetition(Candidate* candidate);

        RandomTracksSource* _randomTracksSource;
        std::mt19937 _randomEngine;
        QueueEntry const* _currentTrack;
        PlayerQueue* _queue;
        Resolver* _resolver;
        History* _history;
        QQueue<Candidate*> _upcoming;
        QTimer* _upcomingTimer;
        uint _upcomingRuntimeSeconds;
        int _noRepetitionSpan;
        int _minimumPermillageByWave;
        quint32 _userPlayingFor;
        bool _enabled;
        bool _refillPending;
        bool _waveActive;
        bool _waveRising;
    };
}
#endif
