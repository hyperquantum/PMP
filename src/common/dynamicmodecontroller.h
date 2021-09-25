/*
    Copyright (C) 2020-2021, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_DYNAMICMODECONTROLLER_H
#define PMP_DYNAMICMODECONTROLLER_H

#include "tribool.h"

#include <QObject>

namespace PMP
{
    class DynamicModeController : public QObject
    {
        Q_OBJECT
    public:
        virtual ~DynamicModeController() {}

        virtual TriBool dynamicModeEnabled() const = 0;
        virtual int noRepetitionSpanSeconds() const = 0;

        virtual TriBool waveActive() const = 0;
        virtual bool canStartWave() const = 0;
        virtual bool canTerminateWave() const = 0;
        virtual int waveProgress() const = 0;
        virtual int waveProgressTotal() const = 0;

    public Q_SLOTS:
        virtual void enableDynamicMode() = 0;
        virtual void disableDynamicMode() = 0;

        virtual void setNoRepetitionSpan(int noRepetitionSpanSeconds) = 0;

        virtual void startHighScoredTracksWave() = 0;
        virtual void terminateHighScoredTracksWave() = 0;

    Q_SIGNALS:
        void dynamicModeEnabledChanged();
        void noRepetitionSpanSecondsChanged();

        void waveActiveChanged();
        void waveProgressChanged();

    protected:
        DynamicModeController(QObject* parent) : QObject(parent) {}
    };
}
#endif
