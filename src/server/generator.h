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

#ifndef PMP_GENERATOR_H
#define PMP_GENERATOR_H

#include <QObject>
#include <QQueue>
#include <QTimer>

namespace PMP {

    class FileHash;
    class History;
    class Queue;
    class QueueEntry;
    class Resolver;

    class Generator : public QObject {
        Q_OBJECT
    public:
        Generator(Queue* queue, Resolver* resolver, History* history);

        bool enabled() const;

        int noRepetitionSpan() const;

    public slots:
        void enable();
        void disable();
        void requestQueueExpansion();

        void setNoRepetitionSpan(int seconds);

        void currentTrackChanged(QueueEntry const* newTrack);
        void setUserPlayingFor(quint32 user);

    Q_SIGNALS:
        void enabledChanged(bool enabled);
        void noRepetitionSpanChanged(int seconds);

    private slots:
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

        void requestQueueRefill();
        int expandQueue(int howManyTracksToAdd, int maxIterations);
        bool satisfiesFilters(Candidate* candidate);

        QueueEntry const* _currentTrack;
        Queue* _queue;
        Resolver* _resolver;
        History* _history;
        bool _enabled;
        bool _refillPending;
        QQueue<Candidate*> _upcoming;
        uint _upcomingRuntimeSeconds;
        QTimer* _upcomingTimer;
        int _noRepetitionSpan;
        quint32 _userPlayingFor;
    };
}
#endif
