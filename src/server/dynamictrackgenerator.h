/*
    Copyright (C) 2014-2022, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_DYNAMICTRACKGENERATOR_H
#define PMP_DYNAMICTRACKGENERATOR_H

#include "trackgeneratorbase.h"

#include <QQueue>

namespace PMP::Server
{
    class DynamicTrackGenerator : public TrackGeneratorBase
    {
        Q_OBJECT
    public:
        DynamicTrackGenerator(QObject* parent, RandomTracksSource* source,
                              Resolver* resolver, History* history,
                              TrackRepetitionChecker* repetitionChecker);

        QVector<FileHash> getTracks(int count) override;

    public Q_SLOTS:
        void enable();
        void disable();

        void freezeTemporarily();

    private Q_SLOTS:
        void upcomingRefillTimerAction();

    private:
        void criteriaChanged() override;
        void desiredUpcomingCountChanged() override;
        void checkIfRefillNeeded();

        int selectionFilterCompare(Candidate const& t1, Candidate const& t2);
        bool satisfiesFilters(Candidate& candidate);

        bool satisfiesBasicFilter(Candidate const& candidate) override;

        QQueue<QSharedPointer<Candidate>> _upcoming;
        bool _enabled;
        bool _refillPending;
        bool _temporaryFreeze;
    };
}
#endif
