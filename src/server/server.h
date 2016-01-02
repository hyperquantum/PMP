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

#ifndef PMP_SERVER_H
#define PMP_SERVER_H

#include <QHostAddress>
#include <QUuid>

QT_FORWARD_DECLARE_CLASS(QTcpServer)

namespace PMP {

    class CollectionMonitor;
    class Generator;
    class Player;
    class Users;

    class Server : public QObject {
        Q_OBJECT
    public:
        Server(QObject* parent, const QUuid& serverInstanceIdentifier);

        bool listen(Player* player, Generator* generator, Users* users,
                    CollectionMonitor* collectionMonitor,
                    const QHostAddress& address = QHostAddress::Any, quint16 port = 0);

        QString errorString() const;

        quint16 port() const;

        QUuid uuid() const { return _uuid; }
        QString serverPassword() { return _serverPassword; }

    public slots:
        void shutdown();

    Q_SIGNALS:
        void shuttingDown();

    private slots:
        void newConnectionReceived();

    private:
        static QString generateServerPassword();

        QUuid _uuid;
        QString _serverPassword;
        Player* _player;
        Generator* _generator;
        Users* _users;
        CollectionMonitor* _collectionMonitor;
        QTcpServer* _server;
    };
}
#endif
