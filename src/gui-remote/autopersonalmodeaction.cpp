/*
    Copyright (C) 2014-2019, Kevin Andre <hyperquantum@gmail.com>

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

#include "autopersonalmodeaction.h"

namespace PMP {

    AutoPersonalModeAction::AutoPersonalModeAction(ServerConnection* connection)
     : QObject(connection),
       _connection(connection), _needToCheck(true),
       _state(ServerConnection::UnknownState),
       _knowUserPlayingFor(false), _publicMode(false)
    {
        connect(
            _connection, &ServerConnection::connected,
            this, &AutoPersonalModeAction::connected
        );
        connect(
            _connection, &ServerConnection::receivedPlayerState,
            this, &AutoPersonalModeAction::receivedPlayerState
        );
        connect(
            _connection, &ServerConnection::receivedUserPlayingFor,
            this, &AutoPersonalModeAction::userPlayingForChanged
        );
    }

    void AutoPersonalModeAction::connected() {
        _state = ServerConnection::UnknownState;
        _knowUserPlayingFor = false;
        _needToCheck = true;
    }

    void AutoPersonalModeAction::receivedPlayerState(int s, quint8 volume,
                                                     quint32 queueLength,
                                                     quint32 nowPlayingQID,
                                                     quint64 nowPlayingPosition)
    {
        (void)volume;
        (void)queueLength;
        (void)nowPlayingQID;
        (void)nowPlayingPosition;

        _state = static_cast<ServerConnection::PlayState>(s);
        check();
    }

    void AutoPersonalModeAction::userPlayingForChanged(quint32 userId, QString login) {
        (void)login;

        _publicMode = userId == 0;
        _knowUserPlayingFor = true;
        check();
    }

    void AutoPersonalModeAction::check() {
        if (!_needToCheck) return;

        if (_state == ServerConnection::UnknownState || !_knowUserPlayingFor) return;

        _needToCheck = false;

        if (_state == ServerConnection::Stopped && _publicMode) {
            _connection->switchToPersonalMode();
            _connection->enableDynamicMode();
        }
    }
}
