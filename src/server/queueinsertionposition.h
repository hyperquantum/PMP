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

#ifndef PMP_SERVER_QUEUEINSERTIONPOSITION_H
#define PMP_SERVER_QUEUEINSERTIONPOSITION_H

#include <QDebug>

namespace PMP::Server
{
    enum class QueueInsertionPosition
    {
        Front, End
    };

    inline QDebug operator<<(QDebug debug, QueueInsertionPosition position)
    {
        switch (position)
        {
        case QueueInsertionPosition::Front:
            return debug << "FRONT";
        case QueueInsertionPosition::End:
            return debug << "END";
        }

        return debug << (int)position;
    }
}
#endif
