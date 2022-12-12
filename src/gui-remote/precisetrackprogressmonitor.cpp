/*
    Copyright (C) 2020-2022, Kevin Andre <hyperquantum@gmail.com>

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

#include "precisetrackprogressmonitor.h"

#include "client/currenttrackmonitor.h"

#include <QTimer>

using namespace PMP::Client;

namespace PMP
{
    PreciseTrackProgressMonitor::PreciseTrackProgressMonitor(
                                                 CurrentTrackMonitor* currentTrackMonitor)
     : QObject(currentTrackMonitor),
       _currentTrackMonitor(currentTrackMonitor),
       _refreshTimer(new QTimer(this)),
       _playerState(PlayerState::Unknown),
       _currentQueueId(0),
       _progressAtTimerStart(0),
       _currentTrackLengthMilliseconds(-1)
    {
        connect(
            _currentTrackMonitor, &CurrentTrackMonitor::trackProgressChanged,
            this, &PreciseTrackProgressMonitor::onTrackProgressReceived
        );

        _refreshTimer->setInterval(TIMER_INTERVAL);
        connect(
            _refreshTimer, &QTimer::timeout,
            this, &PreciseTrackProgressMonitor::onTimeout
        );

        auto state = currentTrackMonitor->playerState();
        auto queueId = currentTrackMonitor->currentQueueId();
        auto progress = currentTrackMonitor->currentTrackProgressMilliseconds();
        auto trackLength = currentTrackMonitor->currentTrackLengthMilliseconds();

        onTrackProgressReceived(state, queueId, progress, trackLength);
    }

    PreciseTrackProgressMonitor::~PreciseTrackProgressMonitor()
    {
        delete _refreshTimer;
    }

    void PreciseTrackProgressMonitor::onTrackProgressReceived(PlayerState state,
                                                         quint32 queueId,
                                                         qint64 progressInMilliseconds,
                                                         qint64 trackLengthInMilliseconds)
    {
        _playerState = state;
        _currentQueueId = queueId;

        if (progressInMilliseconds < 0) {
            _progressTimer.invalidate();
            _progressAtTimerStart = 0;
        }
        else {
            _progressTimer.start();
            _progressAtTimerStart = progressInMilliseconds;
        }

        _currentTrackLengthMilliseconds = trackLengthInMilliseconds;

        if (state == PlayerState::Playing) {
            if (!_refreshTimer->isActive()) {
                _refreshTimer->start();
            }
        }
        else {
            if (_refreshTimer->isActive()) {
                _refreshTimer->stop();
            }
        }

        Q_EMIT trackProgressChanged(_playerState, queueId, progressInMilliseconds,
                                    trackLengthInMilliseconds);
    }

    void PreciseTrackProgressMonitor::onTimeout()
    {
        if (!_progressTimer.isValid()) {
            _refreshTimer->stop();
            return;
        }

        emitCalculatedTrackProgress();
    }

    void PreciseTrackProgressMonitor::emitCalculatedTrackProgress()
    {
        auto length = _currentTrackLengthMilliseconds;

        if (!_progressTimer.isValid()) {
            Q_EMIT trackProgressChanged(_playerState, _currentQueueId, -1, length);
            return;
        }

        auto millisecondsSinceTimerStart = _progressTimer.elapsed();

        auto progress = _progressAtTimerStart + millisecondsSinceTimerStart;

        Q_EMIT trackProgressChanged(_playerState, _currentQueueId, progress, length);
    }

}
