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

#ifndef PMP_WAVETRACKGENERATOR_H
#define PMP_WAVETRACKGENERATOR_H

#include "trackgeneratorbase.h"

#include <QQueue>

namespace PMP {

    class WaveTrackGenerator : public TrackGeneratorBase {
        Q_OBJECT
    public:
        WaveTrackGenerator(QObject* parent, RandomTracksSource* source,
                           Resolver* resolver, History* history,
                           TrackRepetitionChecker* repetitionChecker);

        QVector<FileHash> getTracks(int count) override;

    public Q_SLOTS:
        void startWave();
        void terminateWave();

    Q_SIGNALS:
        void waveStarted();
        void waveEnded();

    private Q_SLOTS:
        void upcomingRefillTimerAction();

    private:
        void growBuffer();
        void applySelectionFilterToBufferAndAppendToUpcoming();

        void criteriaChanged() override;
        void desiredUpcomingCountChanged() override;

        int selectionFilterCompare(Candidate const& t1, Candidate const& t2);

        bool satisfiesBasicFilter(Candidate const& candidate) override;

        QQueue<QSharedPointer<Candidate>> _upcoming;
        QVector<QSharedPointer<Candidate>> _buffer;
        int _trackGenerationProgress;
        bool _waveActive;
        bool _waveGenerationCompleted;
    };
}
#endif
