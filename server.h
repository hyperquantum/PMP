/*
    Copyright (C) 2014, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_SERVER_H
#define PMP_SERVER_H

#include <QByteArray>
#include <QTcpServer>

namespace PMP {

    class Player;

    class Server : public QObject {
        Q_OBJECT
    public:
        explicit Server(QObject* parent = 0);

        bool listen(Player* player);
        QString errorString() const;

        quint16 port() const;

    public slots:

        void shutdown();

    Q_SIGNALS:

        void shuttingDown();

    private slots:

        void newConnectionReceived();


    private:
        Player* _player;
        QTcpServer* _server;
        QByteArray _greeting;
    };
}
#endif
