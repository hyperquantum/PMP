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
        int waveProgress() const;
        int waveProgressTotal() const;

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
        void enabledChanged();
        void noRepetitionSpanChanged();
        void waveStarting();
        void waveProgressChanged(int tracksDelivered, int tracksTotal);
        void waveFinished(bool completed);

    private Q_SLOTS:
        void upcomingTrackNotification(FileHash hash);
        void queueEntryRemoved(quint32, quint32);

        void queueRefillTimerAction();

    private:
        void checkQueueRefillNeeded();
        void setDesiredUpcomingCount();
        void loadAndApplyUserPreferences(quint32 user);
        void saveUserPreferences();

        RandomTracksSource* _randomTracksSource;
        TrackRepetitionChecker* _repetitionChecker;
        DynamicTrackGenerator* _trackGenerator;
        WaveTrackGenerator* _waveTrackGenerator;
        PlayerQueue* _queue;
        Resolver* _resolver;
        History* _history;
        int _waveProgress;
        int _waveProgressTotal;
        DynamicModeCriteria _criteria;
        bool _enabled;
        bool _refillPending;
        bool _waveActive;
    };
}
#endif
