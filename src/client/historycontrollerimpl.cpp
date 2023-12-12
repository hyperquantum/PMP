/*
    Copyright (C) 2022-2023, Kevin Andre <hyperquantum@gmail.com>

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

#include "historycontrollerimpl.h"

#include "serverconnection.h"

namespace PMP::Client
{
    HistoryControllerImpl::HistoryControllerImpl(ServerConnection* connection)
     : HistoryController(connection),
       _connection(connection)
    {
        connect(
            _connection, &ServerConnection::connected,
            this, &HistoryControllerImpl::connected
        );
        connect(
            _connection, &ServerConnection::disconnected,
            this, &HistoryControllerImpl::connectionBroken
        );
        connect(
            _connection, &ServerConnection::receivedPlayerHistoryEntry,
            this, &HistoryControllerImpl::receivedPlayerHistoryEntry
        );
        connect(
            _connection, &ServerConnection::receivedPlayerHistory,
            this, &HistoryControllerImpl::receivedPlayerHistory
        );

        if (_connection->isConnected())
            connected();
    }

    void HistoryControllerImpl::sendPlayerHistoryRequest(int limit)
    {
        _connection->sendPlayerHistoryRequest(limit);
    }

    Future<HistoryFragment, AnyResultMessageCode>
        HistoryControllerImpl::getPersonalTrackHistory(LocalHashId hashId, uint userId,
                                                       int limit, uint startId)
    {
        return _connection->getPersonalTrackHistory(hashId, userId, limit, startId);
    }

    void HistoryControllerImpl::connected()
    {
        //
    }

    void HistoryControllerImpl::connectionBroken()
    {
        //
    }
}
