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

#include "playercontrollerimpl.h"

#include "serverconnection.h"

#include <QtDebug>

namespace PMP {

    PlayerControllerImpl::PlayerControllerImpl(ServerConnection* connection)
     : PlayerController(connection),
       _connection(connection),
       _state(PlayerState::Unknown),
       _queueLength(0),
       _trackNowPlaying(0),
       _trackJustSkipped(0),
       _mode(PlayerMode::Unknown),
       _personalModeUserId(0),
       _volume(-1)
    {
        connect(
            _connection, &ServerConnection::connected,
            this, &PlayerControllerImpl::connected
        );
        connect(
            _connection, &ServerConnection::disconnected,
            this, &PlayerControllerImpl::connectionBroken
        );
        connect(
            _connection, &ServerConnection::receivedPlayerState,
            this, &PlayerControllerImpl::receivedPlayerState
        );
        connect(
            _connection, &ServerConnection::receivedUserPlayingFor,
            this, &PlayerControllerImpl::receivedUserPlayingFor
        );
        connect(
            _connection, &ServerConnection::volumeChanged,
            this, &PlayerControllerImpl::receivedVolume
        );

        if (_connection->isConnected())
            connected();
    }

    PlayerState PlayerControllerImpl::playerState() const {
        return _state;
    }

    TriBool PlayerControllerImpl::isTrackPresent() const
    {
        if (_state == PlayerState::Unknown)
            return TriBool::unknown;

        return _trackNowPlaying > 0;
    }

    quint32 PlayerControllerImpl::currentQueueId() const
    {
        return _trackNowPlaying;
    }

    uint PlayerControllerImpl::queueLength() const
    {
        return _queueLength;
    }

    bool PlayerControllerImpl::canPlay() const {
        return _queueLength > 0
            && (_state == PlayerState::Paused
                || _state == PlayerState::Stopped);
    }

    bool PlayerControllerImpl::canPause() const {
        return _state == PlayerState::Playing;
    }

    bool PlayerControllerImpl::canSkip() const
    {
        /* avoid repeating the skip command */
        if (_trackJustSkipped > 0 && _trackJustSkipped == _trackNowPlaying)
            return false;

        if (_state == PlayerState::Playing || _state == PlayerState::Paused)
            return true;

        return false;
    }

    PlayerMode PlayerControllerImpl::playerMode() const
    {
        return _mode;
    }

    quint32 PlayerControllerImpl::personalModeUserId() const
    {
        return _personalModeUserId;
    }

    QString PlayerControllerImpl::personalModeUserLogin() const
    {
        return _personalModeUserLogin;
    }

    int PlayerControllerImpl::volume() const
    {
        return _volume;
    }

    void PlayerControllerImpl::play() {
        _connection->play();
    }

    void PlayerControllerImpl::pause() {
        _connection->pause();
    }

    void PlayerControllerImpl::skip() {
        _trackJustSkipped = _trackNowPlaying;
        _connection->skip();
    }

    void PlayerControllerImpl::setVolume(int volume)
    {
        _connection->setVolume(qBound(0, volume, 100));
    }

    void PlayerControllerImpl::switchToPublicMode()
    {
        _connection->switchToPublicMode();
    }

    void PlayerControllerImpl::switchToPersonalMode()
    {
        _connection->switchToPersonalMode();
    }

    void PlayerControllerImpl::connected()
    {
        _connection->requestPlayerState();
        _connection->requestUserPlayingForMode();
    }

    void PlayerControllerImpl::connectionBroken()
    {
        updateMode(PlayerMode::Unknown, 0, QString());
        updateState(PlayerState::Unknown, -1, 0, 0,-1);
    }

    void PlayerControllerImpl::receivedPlayerState(PlayerState state, quint8 volume,
                quint32 queueLength, quint32 nowPlayingQID, quint64 nowPlayingPosition)
    {
        updateState(state, volume, queueLength, nowPlayingQID, nowPlayingPosition);
    }

    void PlayerControllerImpl::receivedUserPlayingFor(quint32 userId, QString userLogin)
    {
        if (userId > 0)
            updateMode(PlayerMode::Personal, userId, userLogin);
        else
            updateMode(PlayerMode::Public, 0, QString());
    }

    void PlayerControllerImpl::receivedVolume(int volume)
    {
        if (_volume == volume)
            return;

        _volume = volume;
        qDebug() << "volume changed to" << volume;
        Q_EMIT volumeChanged();
    }

    void PlayerControllerImpl::updateState(PlayerState state, int volume,
                                           quint32 queueLength, quint32 nowPlayingQueueId,
                                           qint64 nowPlayingPosition)
    {
        Q_UNUSED(nowPlayingPosition);

        bool stateChanged = _state != state;
        bool queueLengthChanged = _queueLength != queueLength;
        bool currentQueueIdChanged = _trackNowPlaying != nowPlayingQueueId;
        bool volumeChanged = _volume != volume;

        _state = state;
        _queueLength = queueLength;
        _trackNowPlaying = nowPlayingQueueId;
        _volume = volume;

        if (stateChanged) {
            qDebug() << "player state changed to" << state;
            Q_EMIT playerStateChanged(state);
        }

        if (currentQueueIdChanged)
            Q_EMIT currentTrackChanged();

        if (queueLengthChanged)
            Q_EMIT this->queueLengthChanged();

        if (volumeChanged) {
            qDebug() << "volume changed to" << volume;
            Q_EMIT this->volumeChanged();
        }
    }

    void PlayerControllerImpl::updateMode(PlayerMode mode, quint32 personalModeUserId,
                                          QString personalModeUserLogin)
    {
        if (_mode == mode && _personalModeUserId == personalModeUserId
                          && _personalModeUserLogin == personalModeUserLogin)
        {
            return; /* no change */
        }

        qDebug() << "player mode changed to" << mode;

        _mode = mode;
        _personalModeUserId = personalModeUserId;
        _personalModeUserLogin = personalModeUserLogin;

        Q_EMIT playerModeChanged(mode, personalModeUserId, personalModeUserLogin);
    }

}
