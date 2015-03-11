/*
    Copyright (C) 2014-2015, Kevin Andre <hyperquantum@gmail.com>

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

#include "server.h"

#include "connectedclient.h"

#include <QByteArray>
#include <QTcpServer>
#include <QTcpSocket>

namespace PMP {

    Server::Server(QObject* parent, const QUuid& serverInstanceIdentifier)
     : QObject(parent),
       _uuid(serverInstanceIdentifier),
       _player(0), _server(new QTcpServer(this))
    {
        /* generate a new UUID for ourselves if we did not receive a valid one */
        if (_uuid.isNull()) _uuid = QUuid::createUuid();

        connect(_server, SIGNAL(newConnection()), this, SLOT(newConnectionReceived()));
    }

    bool Server::listen(Player* player, Generator* generator, const QHostAddress& address,
                        quint16 port)
    {
        _player = player;
        _generator = generator;

        if (!_server->listen(QHostAddress::Any, port)) {
            return false;
        }

        return true;
    }

    QString Server::errorString() const {
        return _server->errorString();
    }

    quint16 Server::port() const {
        return _server->serverPort();
    }

    void Server::shutdown() {
        emit shuttingDown();
    }

    void Server::newConnectionReceived() {
        QTcpSocket *connection = _server->nextPendingConnection();

        new ConnectedClient(connection, this, _player, _generator);
    }
}
