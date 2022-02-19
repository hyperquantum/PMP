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

#ifndef PMP_DELAYEDSTARTNOTIFICATION_H
#define PMP_DELAYEDSTARTNOTIFICATION_H

#include "notificationbar.h"

namespace PMP
{
    class GeneralController;
    class PlayerController;

    class DelayedStartNotification : public Notification
    {
        Q_OBJECT
    public:
        DelayedStartNotification(QObject* parent, PlayerController* playerController,
                                 GeneralController* generalController);

        QString notificationText() const override { return _text; }

        QString actionButton1Text() const override;

        bool visible() const override { return _visible; }

        void actionButton1Pushed() override;

    private Q_SLOTS:
        void updateInfo();

    private:
        PlayerController* _playerController;
        GeneralController* _generalController;
        QString _text;
        bool _visible;
    };
}
#endif
