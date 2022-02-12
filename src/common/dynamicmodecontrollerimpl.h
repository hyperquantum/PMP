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

#ifndef PMP_DYNAMICMODECONTROLLERIMPL_H
#define PMP_DYNAMICMODECONTROLLERIMPL_H

#include "dynamicmodecontroller.h"

namespace PMP
{
    class ServerConnection;

    class DynamicModeControllerImpl : public DynamicModeController
    {
        Q_OBJECT
    public:
        explicit DynamicModeControllerImpl(ServerConnection* connection);

        TriBool dynamicModeEnabled() const override;
        int noRepetitionSpanSeconds() const override;

        TriBool waveActive() const override;
        bool canStartWave() const override;
        bool canTerminateWave() const override;
        int waveProgress() const override;
        int waveProgressTotal() const override;

    public Q_SLOTS:
        void enableDynamicMode() override;
        void disableDynamicMode() override;

        void setNoRepetitionSpan(int noRepetitionSpanSeconds) override;

        void startHighScoredTracksWave() override;
        void terminateHighScoredTracksWave() override;

        void expandQueue() override;
        void trimQueue() override;

    private Q_SLOTS:
        void connected();
        void connectionBroken();
        void dynamicModeStatusReceived(bool enabled, int noRepetitionSpanSeconds);
        void dynamicModeHighScoreWaveStatusReceived(bool active, bool statusChanged,
                                                    int progress, int progressTotal);

    private:
        void updateStatus(TriBool enabled, int noRepetitionSpanSeconds);
        void updateWaveStatus(TriBool active, int progress, int progressTotal);

        ServerConnection* _connection;
        TriBool _dynamicModeEnabled;
        TriBool _waveActive;
        int _noRepetitionSpanSeconds;
        int _waveProgress;
        int _waveProgressTotal;
    };
}
#endif
