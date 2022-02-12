/*
    Copyright (C) 2021, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_DISCONNECTREASON_H
#define PMP_DISCONNECTREASON_H

#include <QtDebug>

namespace PMP
{
    enum class DisconnectReason
    {
        Unknown, ClientInitiated, KeepAliveTimeout, ProtocolError, SocketError,
    };

    inline QDebug operator<<(QDebug debug, DisconnectReason reason)
    {
        switch (reason)
        {
        case PMP::DisconnectReason::Unknown:
            debug << "DisconnectReason::Unknown";
            return debug;

        case PMP::DisconnectReason::ClientInitiated:
            debug << "DisconnectReason::ClientInitiated";
            return debug;

        case PMP::DisconnectReason::KeepAliveTimeout:
            debug << "DisconnectReason::KeepAliveTimeout";
            return debug;

        case PMP::DisconnectReason::ProtocolError:
            debug << "DisconnectReason::ProtocolError";
            return debug;

        case PMP::DisconnectReason::SocketError:
            debug << "DisconnectReason::SocketError";
            return debug;
        }

        debug << int(reason);
        return debug;
    }
}

Q_DECLARE_METATYPE(PMP::DisconnectReason)

#endif
