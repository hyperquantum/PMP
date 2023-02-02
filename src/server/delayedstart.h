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

#ifndef PMP_DELAYEDSTART_H
#define PMP_DELAYEDSTART_H

#include "result.h"

#include <QDeadlineTimer>
#include <QObject>

QT_FORWARD_DECLARE_CLASS(QTimer)

namespace PMP::Server
{
    class Player;

    class DelayedStart : public QObject
    {
        Q_OBJECT
    public:
        explicit DelayedStart(Player* player);

        bool isActive() const { return _delayedStartActive; }
        Result activate(int delayMilliseconds);
        Result deactivate();

        qint64 timeRemainingMilliseconds() const;

    Q_SIGNALS:
        void delayedStartActiveChanged();

    private:
        void scheduleTimer();
        int getTimerIntervalForRemainingTime(qint64 remainingMilliseconds);
        void doStart();

        Player* _player;
        QTimer* _timer;
        QDeadlineTimer _startDeadline;
        bool _delayedStartActive;
    };
}
#endif
