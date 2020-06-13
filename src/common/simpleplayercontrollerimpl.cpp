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

#include "simpleplayercontrollerimpl.h"

namespace PMP {

    SimplePlayerControllerImpl::SimplePlayerControllerImpl(ServerConnection* connection)
     : QObject(connection),
       _connection(connection), _state(ServerConnection::UnknownState), _queueLength(0),
       _trackNowPlaying(0), _trackJustSkipped(0)
    {
        connect(
            _connection, &ServerConnection::receivedPlayerState,
            this, &SimplePlayerControllerImpl::receivedPlayerState
        );
    }

    void SimplePlayerControllerImpl::receivedPlayerState(int state, quint8 volume,
                quint32 queueLength, quint32 nowPlayingQID, quint64 nowPlayingPosition)
    {
        (void)volume;
        (void)nowPlayingPosition;
        _state = ServerConnection::PlayState(state);
        _queueLength = queueLength;
        _trackNowPlaying = nowPlayingQID;
    }

    void SimplePlayerControllerImpl::play() {
        _connection->play();
    }

    void SimplePlayerControllerImpl::pause() {
        _connection->pause();
    }

    void SimplePlayerControllerImpl::skip() {
        _trackJustSkipped = _trackNowPlaying;
        _connection->skip();
    }

    bool SimplePlayerControllerImpl::canPlay() {
        return _queueLength > 0
            && (_state == ServerConnection::Paused
                || _state == ServerConnection::Stopped);
    }

    bool SimplePlayerControllerImpl::canPause() {
        return _state == ServerConnection::Playing;
    }

    bool SimplePlayerControllerImpl::canSkip() {
        return
            _trackNowPlaying != _trackJustSkipped
            && (_state == ServerConnection::Playing
                || _state == ServerConnection::Paused);
    }

}
