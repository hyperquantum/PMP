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

#include "generalcontrollerimpl.h"

#include "serverconnection.h"

namespace PMP
{
    GeneralControllerImpl::GeneralControllerImpl(ServerConnection* connection)
     : GeneralController(connection),
       _connection(connection),
       _clientClockTimeOffsetMs(0)
    {
        connect(
            _connection, &ServerConnection::connected,
            this, &GeneralControllerImpl::connected
        );
        connect(
            _connection, &ServerConnection::connectionBroken,
            this, &GeneralControllerImpl::connectionBroken
        );
        connect(
            _connection, &ServerConnection::receivedClientClockTimeOffset,
            this, &GeneralControllerImpl::receivedClientClockTimeOffset
        );
        connect(
            _connection, &ServerConnection::serverSettingsReloadResultEvent,
            this, &GeneralControllerImpl::serverSettingsReloadResultEvent
        );

        if (_connection->isConnected())
            connected();
    }

    qint64 GeneralControllerImpl::clientClockTimeOffsetMs() const
    {
        return _clientClockTimeOffsetMs;
    }

    RequestID GeneralControllerImpl::reloadServerSettings()
    {
        return _connection->reloadServerSettings();
    }

    void GeneralControllerImpl::shutdownServer()
    {
        _connection->shutdownServer();
    }

    void GeneralControllerImpl::connected()
    {
        //
    }

    void GeneralControllerImpl::connectionBroken()
    {
        //
    }

    void GeneralControllerImpl::receivedClientClockTimeOffset(
                                                          quint64 clientClockTimeOffsetMs)
    {
        if (clientClockTimeOffsetMs == _clientClockTimeOffsetMs)
            return;

        _clientClockTimeOffsetMs = clientClockTimeOffsetMs;
        Q_EMIT clientClockTimeOffsetChanged();
    }

}
