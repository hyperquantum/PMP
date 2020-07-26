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

#include <QtDebug>

namespace PMP {

    SimplePlayerStateMonitorImpl::SimplePlayerStateMonitorImpl(
                                                             ServerConnection* connection)
     : SimplePlayerStateMonitor(connection),
       _connection(connection),
       _state(PlayerState::Unknown),
       _mode(PlayerMode::Unknown),
       _personalModeUserId(0)
    {
        connect(
            _connection, &ServerConnection::connected,
            this, &SimplePlayerStateMonitorImpl::connected
        );
        connect(
            _connection, &ServerConnection::connectionBroken,
            this, &SimplePlayerStateMonitorImpl::connectionBroken
        );
        connect(
            _connection, &ServerConnection::receivedPlayerState,
            this, &SimplePlayerStateMonitorImpl::receivedPlayerState
        );
        connect(
            _connection, &ServerConnection::receivedUserPlayingFor,
            this, &SimplePlayerStateMonitorImpl::receivedUserPlayingFor
        );

        if (_connection->isConnected())
            connected();
    }

    PlayerState SimplePlayerStateMonitorImpl::playerState() const {
        return _state;
    }

    PlayerMode SimplePlayerStateMonitorImpl::playerMode() const
    {
        return _mode;
    }

    quint32 SimplePlayerStateMonitorImpl::personalModeUserId() const
    {
        return _personalModeUserId;
    }

    QString SimplePlayerStateMonitorImpl::personalModeUserLogin() const
    {
        return _personalModeUserLogin;
    }

    void SimplePlayerStateMonitorImpl::connected()
    {
        _connection->requestPlayerState();
        _connection->requestUserPlayingForMode();
    }

    void SimplePlayerStateMonitorImpl::connectionBroken()
    {
        changeCurrentState(PlayerState::Unknown);
        changeCurrentMode(PlayerMode::Unknown, 0, QString());
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

        changeCurrentState(newState);
    }

    void SimplePlayerStateMonitorImpl::receivedUserPlayingFor(quint32 userId,
                                                              QString userLogin)
    {
        if (userId > 0)
            changeCurrentMode(PlayerMode::Personal, userId, userLogin);
        else
            changeCurrentMode(PlayerMode::Public, 0, QString());
    }

    void SimplePlayerStateMonitorImpl::changeCurrentState(PlayerState state)
    {
        if (state == _state)
            return;

        _state = state;
        Q_EMIT playerStateChanged(state);
    }

    void SimplePlayerStateMonitorImpl::changeCurrentMode(PlayerMode mode,
                                                         quint32 personalModeUserId,
                                                         QString personalModeUserLogin)
    {
        if (_mode == mode && _personalModeUserId == personalModeUserId
                          && _personalModeUserLogin == personalModeUserLogin)
        {
            return; /* no change */
        }

        qDebug() << "player mode changed to:" << mode;
        _mode = mode;
        _personalModeUserId = personalModeUserId;
        _personalModeUserLogin = personalModeUserLogin;

        Q_EMIT playerModeChanged(mode, personalModeUserId, personalModeUserLogin);
    }

}
