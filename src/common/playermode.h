/*
    Copyright (C) 2020-2021, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_PLAYERMODE_H
#define PMP_PLAYERMODE_H

#include <QtDebug>

namespace PMP
{
    enum class PlayerMode
    {
        Unknown = 0,
        Personal,
        Public
    };

    inline QDebug operator<<(QDebug debug, PlayerMode mode)
    {
        switch (mode)
        {
            case PlayerMode::Unknown:
                debug << "PlayerMode::Unknown";
                return debug;

            case PlayerMode::Personal:
                debug << "PlayerMode::Personal";
                return debug;

            case PlayerMode::Public:
                debug << "PlayerMode::Public";
                return debug;
        }

        debug << int(mode);
        return debug;
    }
}

Q_DECLARE_METATYPE(PMP::PlayerMode)

#endif
