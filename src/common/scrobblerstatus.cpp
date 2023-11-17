/*
    Copyright (C) 2019-2022, Kevin Andre <hyperquantum@gmail.com>

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

#include "scrobblerstatus.h"

#include <QDebug>

namespace PMP
{
    QDebug operator<<(QDebug debug, ScrobblerStatus status)
    {
        switch (status)
        {
            case ScrobblerStatus::Red:
                debug << "ScrobblerStatus::Red";
                return debug;

            case ScrobblerStatus::Yellow:
                debug << "ScrobblerStatus::Yellow";
                return debug;

            case ScrobblerStatus::Green:
                debug << "ScrobblerStatus::Green";
                return debug;

            case ScrobblerStatus::WaitingForUserCredentials:
                debug << "ScrobblerStatus::WaitingForUserCredentials";
                return debug;

            case ScrobblerStatus::Unknown:
                debug << "ScrobblerStatus::Unknown";
                return debug;
        }

        debug << int(status);
        return debug;
    }
}
