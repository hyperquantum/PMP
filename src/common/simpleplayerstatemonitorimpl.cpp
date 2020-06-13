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

#include "simpleplayerstatemonitorimpl.h"

#include "serverconnection.h"

namespace PMP {

    SimplePlayerStateMonitorImpl::SimplePlayerStateMonitorImpl(
                                                             ServerConnection* connection)
     : SimplePlayerStateMonitor(connection),
       _connection(connection), _state(PlayerState::Unknown)
    {
        connect(
            _connection, &ServerConnection::receivedPlayerState,
            this, &SimplePlayerStateMonitorImpl::receivedPlayerState
        );
    }

    PlayerState SimplePlayerStateMonitorImpl::playerState() const {
        return _state;
    }

    void SimplePlayerStateMonitorImpl::receivedPlayerState(int state, quint8 volume,
                quint32 queueLength, quint32 nowPlayingQID, quint64 nowPlayingPosition)
    {
        Q_UNUSED(volume)
        Q_UNUSED(queueLength)
        Q_UNUSED(nowPlayingQID)
        Q_UNUSED(nowPlayingPosition)

        auto newState = PlayerState::Unknown;
        switch (ServerConnection::PlayState(state)) {
            case ServerConnection::UnknownState:
                newState = PlayerState::Unknown;
                break;
            case ServerConnection::Stopped:
                newState = PlayerState::Stopped;
                break;
            case ServerConnection::Playing:
                newState = PlayerState::Playing;
                break;
            case ServerConnection::Paused:
                newState = PlayerState::Paused;
                break;
        }

        if (newState == _state)
            return;

        _state = newState;
        emit playerStateChanged(newState);
    }

}
