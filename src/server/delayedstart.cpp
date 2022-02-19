/*
    Copyright (C) 2022, Kevin Andre <hyperquantum@gmail.com>

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

#include "delayedstart.h"

#include "player.h"

#include <QtDebug>
#include <QTimer>

namespace PMP
{
    namespace
    {
        const int maxMillisecondsRemainingForEarlyStart = 10;
        const int minimumTimerIntervalMilliseconds = 8;
    }

    DelayedStart::DelayedStart(Player* player)
     : QObject(player),
       _player(player),
       _timer(new QTimer()),
       _delayedStartActive(false)
    {
        _timer->setSingleShot(true);
        connect(_timer, &QTimer::timeout, this, &DelayedStart::scheduleTimer);

        connect(
            _player, &Player::stateChanged,
            this,
            [this]()
            {
                if (isActive() && _player->state() == ServerPlayerState::Playing)
                {
                    qDebug() << "deactivating delayed start because player just started";
                    deactivate();
                }
            }
        );
    }

    Result DelayedStart::activate(int delayMilliseconds)
    {
        if (delayMilliseconds <= 0)
            return Error::delayOutOfRange();

        if (_delayedStartActive)
            return Error::operationAlreadyRunning();

        qDebug() << "activating delayed start; delay:" << delayMilliseconds << "ms";
        _delayedStartActive = true;
        _startDeadline.setRemainingTime(delayMilliseconds);
        scheduleTimer();

        Q_EMIT delayedStartActiveChanged();
        return Success();
    }

    Result DelayedStart::deactivate()
    {
        if (!_delayedStartActive)
            return NoOp();

        _delayedStartActive = false;
        _timer->stop();
        Q_EMIT delayedStartActiveChanged();
        return Success();
    }

    qint64 DelayedStart::timeRemainingMilliseconds() const
    {
        if (!_delayedStartActive)
            return -1;

        return  _startDeadline.remainingTime();
    }

    void DelayedStart::scheduleTimer()
    {
        _timer->stop(); /* just in case something completely new is scheduled */

        if (!_delayedStartActive)
            return;

        auto remainingTimeMilliseconds = _startDeadline.remainingTime();
        if (remainingTimeMilliseconds <= maxMillisecondsRemainingForEarlyStart)
        {
            doStart();
            return;
        }

        auto interval = getTimerIntervalForRemainingTime(remainingTimeMilliseconds);
        qDebug() << "delayed start: setting timer for" << interval << "ms";
        _timer->start(interval);
    }

    int DelayedStart::getTimerIntervalForRemainingTime(qint64 remainingMilliseconds)
    {
        if (remainingMilliseconds <= 10 * maxMillisecondsRemainingForEarlyStart)
        {
            int interval = remainingMilliseconds - maxMillisecondsRemainingForEarlyStart;
            return qMax(minimumTimerIntervalMilliseconds, interval);
        }

        /* one hour maximum */
        const int oneHour = 60 * 60 * 1000;
        if (remainingMilliseconds >= 2 * oneHour)
            return oneHour;

        return remainingMilliseconds / 2;
    }

    void DelayedStart::doStart()
    {
        qDebug() << "delayed start has reached its deadline and will now start playing";

        /* do this before starting the player, so we avoid triggering the auto-deactivate
           logic */
        _delayedStartActive = false;

        _player->play();

        Q_EMIT delayedStartActiveChanged();
        // TODO : report success/failure
    }

}
