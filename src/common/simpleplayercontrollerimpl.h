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

#ifndef PMP_SIMPLEPLAYERCONTROLLERIMPL_H
#define PMP_SIMPLEPLAYERCONTROLLERIMPL_H

#include "serverconnection.h"
#include "simpleplayercontroller.h"

#include <QObject>

namespace PMP {

    class ServerConnection;

    class SimplePlayerControllerImpl : public QObject, public SimplePlayerController {
        Q_OBJECT
    public:
        SimplePlayerControllerImpl(ServerConnection* connection);

        ~SimplePlayerControllerImpl() {}

        void play();
        void pause();
        void skip();

        bool canPlay();
        bool canPause();
        bool canSkip();

    private slots:
        void receivedPlayerState(int state, quint8 volume, quint32 queueLength,
                                 quint32 nowPlayingQID, quint64 nowPlayingPosition);

    private:
        ServerConnection* _connection;
        ServerConnection::PlayState _state;
        uint _queueLength;
        quint32 _trackNowPlaying;
        quint32 _trackJustSkipped;
    };
}
#endif
