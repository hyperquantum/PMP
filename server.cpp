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

#include "server.h"

#include "connectedclient.h"

#include <QByteArray>
#include <QTcpSocket>

namespace PMP {

    Server::Server(QObject* parent)
     : QObject(parent), _player(0), _server(new QTcpServer(this))
    {
        connect(_server, SIGNAL(newConnection()), this, SLOT(newConnectionReceived()));
    }

    bool Server::listen(Player* player) {
        _player = player;

        // prepare greeting, is fixed for the player, so we create now
        _greeting.clear();
        QTextStream stream(&_greeting, QIODevice::WriteOnly);
        stream.setGenerateByteOrderMark(false);
        stream.setCodec("UTF-8");
        stream << "PMP 0.1 Welcome!;"; // TODO: get player version from Player instance
        stream.flush();

        if (!_server->listen()) {
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

        new ConnectedClient(connection, this, _player);

        // send greeting with PMP version
        connection->write(_greeting);
    }



}
