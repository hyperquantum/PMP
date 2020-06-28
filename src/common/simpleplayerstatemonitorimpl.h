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

#ifndef PMP_SIMPLEPLAYERSTATEMONITORIMPL_H
#define PMP_SIMPLEPLAYERSTATEMONITORIMPL_H

#include "simpleplayerstatemonitor.h"

namespace PMP {

    class ServerConnection;

    class SimplePlayerStateMonitorImpl : public SimplePlayerStateMonitor {
        Q_OBJECT
    public:
        SimplePlayerStateMonitorImpl(ServerConnection* connection);

        PlayerState playerState() const override;

        PlayerMode playerMode() const override;
        quint32 personalModeUserId() const override;
        QString personalModeUserLogin() const override;

    private slots:
        void connected();
        void connectionBroken();
        void receivedPlayerState(int state, quint8 volume, quint32 queueLength,
                                 quint32 nowPlayingQID, quint64 nowPlayingPosition);
        void receivedUserPlayingFor(quint32 userId, QString userLogin);

    private:
        void changeCurrentState(PlayerState state);
        void changeCurrentMode(PlayerMode mode, quint32 personalModeUserId,
                               QString personalModeUserLogin);

        ServerConnection* _connection;
        PlayerState _state;
        PlayerMode _mode;
        quint32 _personalModeUserId;
        QString _personalModeUserLogin;
    };
}
#endif
