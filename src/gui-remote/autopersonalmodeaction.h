/*
    Copyright (C) 2014-2016, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_AUTOPERSONALMODEACTION_H
#define PMP_AUTOPERSONALMODEACTION_H

#include "common/serverconnection.h"

#include <QObject>
#include <QString>
//#include <QtGlobal>

namespace PMP {

    class AutoPersonalModeAction : public QObject {
        Q_OBJECT
    public:
        AutoPersonalModeAction(ServerConnection* connection);

    private slots:
        void connected();

        void receivedPlayerState(int state, quint8 volume, quint32 queueLength,
                                 quint32 nowPlayingQID, quint64 nowPlayingPosition);

        void userPlayingForChanged(quint32 userId, QString login);

    private:
        void check();

        ServerConnection* _connection;
        bool _needToCheck;
        ServerConnection::PlayState _state;
        bool _knowUserPlayingFor;
        bool _publicMode;
    };
}
#endif // PMP_AUTOPERSONALMODEACTION_H
