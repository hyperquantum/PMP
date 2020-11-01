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

#include "dynamicmodecriteria.h"

#include <QObject>
#include <QSet>
#include <QSharedPointer>
#include <QTimer>
#include <QVector>

namespace PMP {

    class DynamicTrackGenerator;
    class History;
    class PlayerQueue;
    class QueueEntry;
    class RandomTracksSource;
    class Resolver;
    class TrackRepetitionChecker;
    class WaveTrackGenerator;

    class Generator : public QObject {
        Q_OBJECT
    public:
        Generator(PlayerQueue* queue, Resolver* resolver, History* history);

        bool enabled() const;
        bool waveActive() const;

        quint32 userPlayingFor() const;

        int noRepetitionSpanSeconds() const;

        History& history();

    public Q_SLOTS:
        void enable();
        void disable();
        void requestQueueExpansion();

        void startWave();
        void terminateWave();

        void currentTrackChanged(QueueEntry const* newTrack);
        void setUserGeneratingFor(quint32 user);

        void setNoRepetitionSpanSeconds(int seconds);

    Q_SIGNALS:
        void enabledChanged(bool enabled);
        void noRepetitionSpanChanged(int seconds);
        void waveStarting(quint32 user);
        void waveFinished(quint32 user);

    private Q_SLOTS:
        void upcomingTrackNotification(FileHash hash);
        void queueEntryRemoved(quint32, quint32);

        void queueRefillTimerAction();

    private:
        void checkQueueRefillNeeded();
        void setDesiredUpcomingCount();
        void advanceWave();
//        bool satisfiesWaveFilter(Candidate* candidate);

        RandomTracksSource* _randomTracksSource;
        TrackRepetitionChecker* _repetitionChecker;
        DynamicTrackGenerator* _trackGenerator;
        WaveTrackGenerator* _waveTrackGenerator;
        PlayerQueue* _queue;
        Resolver* _resolver;
        History* _history;
        //int _minimumPermillageByWave;
        DynamicModeCriteria _criteria;
        bool _enabled;
        bool _refillPending;
        bool _waveActive;
        //bool _waveRising;
    };

}
#endif
