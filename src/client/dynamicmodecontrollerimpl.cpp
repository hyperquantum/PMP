/*
    Copyright (C) 2020-2023, Kevin Andre <hyperquantum@gmail.com>

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

#include "dynamicmodecontrollerimpl.h"

#include "servercapabilities.h"
#include "serverconnection.h"

namespace PMP::Client
{
    DynamicModeControllerImpl::DynamicModeControllerImpl(ServerConnection* connection)
     : DynamicModeController(connection),
       _connection(connection),
       _noRepetitionSpanSeconds(-1),
       _waveProgress(-1),
       _waveProgressTotal(-1)
    {
        connect(
            _connection, &ServerConnection::connected,
            this, &DynamicModeControllerImpl::connected
        );
        connect(
            _connection, &ServerConnection::disconnected,
            this, &DynamicModeControllerImpl::connectionBroken
        );
        connect(
            _connection, &ServerConnection::dynamicModeStatusReceived,
            this, &DynamicModeControllerImpl::dynamicModeStatusReceived
        );
        connect(
            _connection, &ServerConnection::dynamicModeHighScoreWaveStatusReceived,
            this, &DynamicModeControllerImpl::dynamicModeHighScoreWaveStatusReceived
        );

        if (_connection->isConnected())
            connected();
    }

    TriBool DynamicModeControllerImpl::dynamicModeEnabled() const
    {
        return _dynamicModeEnabled;
    }

    int DynamicModeControllerImpl::noRepetitionSpanSeconds() const
    {
        return _noRepetitionSpanSeconds;
    }

    TriBool DynamicModeControllerImpl::waveActive() const
    {
        return _waveActive;
    }

    bool DynamicModeControllerImpl::canStartWave() const
    {
        return _waveActive.isFalse();
    }

    bool DynamicModeControllerImpl::canTerminateWave() const
    {
        return _waveActive.isTrue()
                && _connection->serverCapabilities().supportsDynamicModeWaveTermination();
    }

    int DynamicModeControllerImpl::waveProgress() const
    {
        return _waveProgress;
    }

    int DynamicModeControllerImpl::waveProgressTotal() const
    {
        return _waveProgressTotal;
    }

    void DynamicModeControllerImpl::setDynamicModeEnabled(bool enabled)
    {
        if (enabled)
            enableDynamicMode();
        else
            disableDynamicMode();
    }

    void DynamicModeControllerImpl::enableDynamicMode()
    {
        _connection->enableDynamicMode();
    }

    void DynamicModeControllerImpl::disableDynamicMode()
    {
        _connection->disableDynamicMode();
    }

    void DynamicModeControllerImpl::setNoRepetitionSpan(int noRepetitionSpanSeconds)
    {
        _connection->setDynamicModeNoRepetitionSpan(noRepetitionSpanSeconds);
    }

    void DynamicModeControllerImpl::startHighScoredTracksWave()
    {
        _connection->startDynamicModeWave();
    }

    void DynamicModeControllerImpl::terminateHighScoredTracksWave()
    {
        _connection->terminateDynamicModeWave();
    }

    void DynamicModeControllerImpl::expandQueue()
    {
        _connection->expandQueue();
    }

    void DynamicModeControllerImpl::trimQueue()
    {
        _connection->trimQueue();
    }

    void DynamicModeControllerImpl::connected()
    {
        _connection->requestDynamicModeStatus();
    }

    void DynamicModeControllerImpl::connectionBroken()
    {
        updateStatus(TriBool::unknown, -1);
        updateWaveStatus(TriBool::unknown, -1, 0);
    }

    void DynamicModeControllerImpl::dynamicModeStatusReceived(bool enabled,
                                                              int noRepetitionSpanSeconds)
    {
        updateStatus(enabled, noRepetitionSpanSeconds);
    }

    void DynamicModeControllerImpl::dynamicModeHighScoreWaveStatusReceived(bool active,
                                                                       bool statusChanged,
                                                                       int progress,
                                                                       int progressTotal)
    {
        Q_UNUSED(statusChanged)

        updateWaveStatus(active, progress, progressTotal);
    }

    void DynamicModeControllerImpl::updateStatus(TriBool enabled,
                                                 int noRepetitionSpanSeconds)
    {
        bool enabledChanged = !_dynamicModeEnabled.isIdenticalTo(enabled);
        bool spanChanged = _noRepetitionSpanSeconds != noRepetitionSpanSeconds;

        _dynamicModeEnabled = enabled;
        _noRepetitionSpanSeconds = noRepetitionSpanSeconds;

        if (enabledChanged)
        {
            Q_EMIT dynamicModeEnabledChanged();
        }

        if (spanChanged)
        {
            Q_EMIT noRepetitionSpanSecondsChanged();
        }
    }

    void DynamicModeControllerImpl::updateWaveStatus(TriBool active,
                                                     int progress, int progressTotal)
    {
        bool activeChanged = !_waveActive.isIdenticalTo(active);
        bool progressChanged =
                _waveProgress != progress || _waveProgressTotal != progressTotal;

        _waveActive = active;
        _waveProgress = progress;
        _waveProgressTotal = progressTotal;

        if (activeChanged || progressChanged)
        {
            Q_EMIT waveStatusChanged();
        }
    }
}
