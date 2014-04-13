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

#include "serverconnection.h"

#include <QtDebug>

namespace PMP {

    ServerConnection::ServerConnection(QObject* parent)
     : QObject(parent), _state(ServerConnection::NotConnected)
    {
        connect(&_socket, SIGNAL(connected()), this, SLOT(onConnected()));
        connect(&_socket, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
        connect(&_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onSocketError(QAbstractSocket::SocketError)));
    }

    void ServerConnection::connectToHost(QString const& host, quint16 port) {
        qDebug() << "connecting to" << host << "on port" << port;
        _state = Connecting;
        _socket.connectToHost(host, port);
    }

    void ServerConnection::onConnected() {
        qDebug() << "connected to host";
        _state = Handshake;
    }

    void ServerConnection::onReadyRead() {
        _readBuffer.append(_socket.readAll());
        int bufLen = _readBuffer.length();

        switch (_state) {
        case NotConnected:
            break; /* problem */
        case Connecting: /* fall-through */
        case Handshake:
            if ((bufLen >= 1 && _readBuffer[0] != 'P')
                || (bufLen >= 2 && _readBuffer[1] != 'M')
                || (bufLen >= 3 && _readBuffer[2] != 'P'))
            {
                _state = HandshakeFailure;
                break;
            }

            {
                int semicolonIndex = _readBuffer.indexOf(';');

                /* TODO: other checks */

                if (semicolonIndex >= 0) {
                    QString serverHelloString = QString::fromUtf8(_readBuffer.data(), semicolonIndex);
                    qDebug() << "server hello:" << serverHelloString;

                    /* TODO: extract server version */

                    _readBuffer.remove(0, semicolonIndex + 1); /* +1 to remove the semicolon too */
                    _state = InOperation;
                    emit connected();
                }
            }
            break;
        case InOperation:

            break;
        case HandshakeFailure:
            /* do nothing */
            break;
        }
    }

    void ServerConnection::onSocketError(QAbstractSocket::SocketError error) {
        qDebug() << "socket error" << error;

        switch (error) {
        case QAbstractSocket::RemoteHostClosedError:

            break;

        default:

            break;
        }
    }

    void ServerConnection::sendTextCommand(QString const& command) {
        qDebug() << "sending command" << command;
        _socket.write((command + ";").toUtf8());
    }

    void ServerConnection::play() {
        sendTextCommand("play");
    }

    void ServerConnection::pause() {
        sendTextCommand("pause");
    }

    void ServerConnection::skip() {
        sendTextCommand("skip");
    }

}
