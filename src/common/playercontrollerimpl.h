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

#ifndef PMP_PLAYERCONTROLLERIMPL_H
#define PMP_PLAYERCONTROLLERIMPL_H

#include "playercontroller.h"

namespace PMP {

    class ServerConnection;

    class PlayerControllerImpl : public PlayerController {
        Q_OBJECT
    public:
        PlayerControllerImpl(ServerConnection* connection);

        ~PlayerControllerImpl() {}

        PlayerState playerState() const override;
        uint queueLength() const override;
        bool canPlay() const override;
        bool canPause() const override;
        bool canSkip() const override;

        PlayerMode playerMode() const override;
        quint32 personalModeUserId() const override;
        QString personalModeUserLogin() const override;

        int volume() const override;

    public Q_SLOTS:
        void play() override;
        void pause() override;
        void skip() override;

        void setVolume(int volume) override;

        void switchToPublicMode() override;
        void switchToPersonalMode() override;

    private Q_SLOTS:
        void connected();
        void connectionBroken();
        void receivedPlayerState(PlayerState state, quint8 volume, quint32 queueLength,
                                 quint32 nowPlayingQID, quint64 nowPlayingPosition);
        void receivedUserPlayingFor(quint32 userId, QString userLogin);
        void receivedVolume(int volume);

    private:
        void updateState(PlayerState state, int volume, quint32 queueLength,
                         quint32 nowPlayingQueueId, qint64 nowPlayingPosition);
        void updateMode(PlayerMode mode, quint32 personalModeUserId,
                        QString personalModeUserLogin);

        ServerConnection* _connection;
        PlayerState _state;
        uint _queueLength;
        quint32 _trackNowPlaying;
        quint32 _trackJustSkipped;
        PlayerMode _mode;
        quint32 _personalModeUserId;
        QString _personalModeUserLogin;
        int _volume;
    };
}
#endif
