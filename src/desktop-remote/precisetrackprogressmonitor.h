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

#ifndef PMP_PRECISETRACKPROGRESSMONITOR_H
#define PMP_PRECISETRACKPROGRESSMONITOR_H

#include "common/playerstate.h"

#include <QElapsedTimer>
#include <QObject>

QT_FORWARD_DECLARE_CLASS(QTimer)

namespace PMP::Client
{
    class CurrentTrackMonitor;
}

namespace PMP
{
    class PreciseTrackProgressMonitor : public QObject {
        Q_OBJECT
    public:
        PreciseTrackProgressMonitor(Client::CurrentTrackMonitor* currentTrackMonitor);

        ~PreciseTrackProgressMonitor();

    Q_SIGNALS:
        void trackProgressChanged(PlayerState state, quint32 queueId,
                                  qint64 progressInMilliseconds,
                                  qint64 trackLengthInMilliseconds);

    private Q_SLOTS:
        void onTrackProgressReceived(PlayerState state, quint32 queueId,
                                     qint64 progressInMilliseconds,
                                     qint64 trackLengthInMilliseconds);
        void onTimeout();

    private:
        void emitCalculatedTrackProgress();

        static const int TIMER_INTERVAL = 40; /* 25 times per second */

        Client::CurrentTrackMonitor* _currentTrackMonitor;
        QTimer* _refreshTimer;
        PlayerState _playerState;
        quint32 _currentQueueId;
        QElapsedTimer _progressTimer;
        qint64 _progressAtTimerStart;
        qint64 _currentTrackLengthMilliseconds;
    };
}
#endif
