/*
    Copyright (C) 2025, Kevin André <hyperquantum@gmail.com>

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

#include "inactivitytimer.h"

#include <QtDebug>
#include <QTimer>

namespace PMP::Client
{
    InactivityTimer::InactivityTimer(QObject* parent)
     : QObject(parent),
        _timer(new QTimer(this))
    {
        _timer->setSingleShot(true);

        connect(
            _timer, &QTimer::timeout,
            this, &InactivityTimer::onTimerTimeout
        );
    }

    void InactivityTimer::start()
    {
        if (_started)
        {
            qDebug() << "InactivityTimer: restarting";

            _timer->stop();
        }

        _started = true;
        _waitingForSecondTimeout = false;
        _timer->start(KeepAliveIntervalMs);
    }

    void InactivityTimer::stop()
    {
        if (!_started)
            return;

        _started = false;
        _timer->stop();
    }

    void InactivityTimer::reportActivity()
    {
        if (!_started)
            return;

        _timer->stop();

        _waitingForSecondTimeout = false;
        _timer->start(KeepAliveIntervalMs);
    }

    void InactivityTimer::onTimerTimeout()
    {
        if (!_started)
            return;

        if (!_waitingForSecondTimeout)
        {
            qDebug()
                << "InactivityTimer: no activity for a while - need to send keep-alive";

            _waitingForSecondTimeout = true;
            _secondTimeoutTimePassedMs = 0;
            _timer->start(SecondTimeoutStepTimeMs);

            Q_EMIT keepAliveTimeout();
            return;
        }

        _secondTimeoutTimePassedMs += SecondTimeoutStepTimeMs;

        if (_secondTimeoutTimePassedMs < SecondTimeoutMaximumTimeMs)
        {
            _timer->start(SecondTimeoutStepTimeMs);
            return;
        }

        qDebug() << "InactivityTimer: still no activity - maximum waiting time reached";

        _started = false;

        Q_EMIT inactivityTimeout();
    }
}
