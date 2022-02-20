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

#include "delayedstartnotification.h"

#include "common/generalcontroller.h"
#include "common/playercontroller.h"
#include "common/util.h"

#include <QTimer>

namespace PMP
{
    DelayedStartNotification::DelayedStartNotification(QObject* parent,
                                                       PlayerController* playerController,
                                                     GeneralController* generalController)
     : Notification(parent),
       _playerController(playerController),
       _generalController(generalController),
       _countDownTimer(new QTimer(this)),
       _visible(false)
    {
        connect(
            _playerController, &PlayerController::delayedStartActiveInfoChanged,
            this, &DelayedStartNotification::updateInfo
        );

        connect(
            _generalController, &GeneralController::clientClockTimeOffsetChanged,
            this, &DelayedStartNotification::updateInfo
        );

        _countDownTimer->setSingleShot(true);
        connect(
            _countDownTimer, &QTimer::timeout,
            this, &DelayedStartNotification::updateInfo
        );

        updateInfo();
    }

    QString DelayedStartNotification::actionButton1Text() const
    {
        return "Deactivate";
    }

    void DelayedStartNotification::actionButton1Pushed()
    {
        _playerController->deactivateDelayedStart();
    }

    void DelayedStartNotification::updateInfo()
    {
        bool active = _playerController->delayedStartActive().isTrue();

        QString text;
        if (active)
        {
            auto deadline = _playerController->delayedStartServerDeadline();

            if (deadline.isNull())
            {
                text = "Delayed start is active";
            }
            else
            {
                deadline =
                        deadline.addMSecs(_generalController->clientClockTimeOffsetMs());

                auto timeRemainingMs =
                        qMax(0LL, QDateTime::currentDateTimeUtc().msecsTo(deadline));

                text =
                    QString("Delayed start active - will start at %1 - time remaining %2")
                        .arg(deadline.toLocalTime().toString("HH:mm:ss"),
                             Util::getCountdownTimeText(timeRemainingMs));

                auto updateIntervalMs =
                        Util::getCountdownUpdateIntervalMs(timeRemainingMs);

                _countDownTimer->start(updateIntervalMs);
            }
        }

        if (text != _text)
        {
            _text = text;
            Q_EMIT notificationTextChanged();
        }

        if (active != _visible)
        {
            _visible = active;
            Q_EMIT visibleChanged();
        }
    }

}
