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

#ifndef PMP_PLAYERSTATE_H
#define PMP_PLAYERSTATE_H

#include <QtDebug>

namespace PMP {

    enum class PlayerState {
        Unknown = 0,
        Stopped,
        Playing,
        Paused
    };

    inline QDebug operator<<(QDebug debug, PlayerState state) {
        switch (state) {
            case PlayerState::Unknown:
                debug << "PlayerState::Unknown";
                return debug;

            case PlayerState::Stopped:
                debug << "PlayerState::Stopped";
                return debug;

            case PlayerState::Playing:
                debug << "PlayerState::Playing";
                return debug;

            case PlayerState::Paused:
                debug << "PlayerState::Paused";
                return debug;
        }

        debug << int(state);
        return debug;
    }
}

Q_DECLARE_METATYPE(PMP::PlayerState)

#endif
