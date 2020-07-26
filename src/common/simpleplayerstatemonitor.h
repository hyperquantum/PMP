/*
    Copyright (C) 2020, Kevin Andre <hyperquantum@gmail.com>

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

#include <QObject>

#ifndef PMP_SIMPLEPLAYERSTATEMONITOR_H
#define PMP_SIMPLEPLAYERSTATEMONITOR_H

#include "playermode.h"
#include "playerstate.h"
#include "tribool.h"

#include <QString>

namespace PMP {

    class SimplePlayerStateMonitor : public QObject {
        Q_OBJECT
    public:
        virtual ~SimplePlayerStateMonitor() {}

        virtual PlayerState playerState() const = 0;

        virtual PlayerMode playerMode() const = 0;
        virtual quint32 personalModeUserId() const = 0;
        virtual QString personalModeUserLogin() const = 0;

    Q_SIGNALS:
        void playerStateChanged(PlayerState playerState);
        void playerModeChanged(PlayerMode playerMode, quint32 personalModeUserId,
                               QString personalModeUserLogin);

    protected:
        explicit SimplePlayerStateMonitor(QObject* parent) : QObject(parent) {}
    };

}
#endif
