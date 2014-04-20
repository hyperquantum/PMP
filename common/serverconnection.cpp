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

    void ServerConnection::reset() {
        _state = NotConnected;
        _socket.abort();
        _readBuffer.clear();
    }

    void ServerConnection::connectToHost(QString const& host, quint16 port) {
        qDebug() << "connecting to" << host << "on port" << port;
        _state = Connecting;
        _readBuffer.clear();
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
                emit invalidServer();
                reset();
                return;
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

                    requestInitialInfo();

                    emit connected();
                }
            }
            break;
        case InOperation:
            do {
                int semicolonIndex = _readBuffer.indexOf(';');
                if (semicolonIndex < 0) { break; }

                QString commandString = QString::fromUtf8(_readBuffer.data(), semicolonIndex);

                _readBuffer.remove(0, semicolonIndex + 1); /* +1 to remove the semicolon too */

                executeTextCommand(commandString);
            } while (true);
            break;
        case HandshakeFailure:
            /* do nothing */
            break;
        }
    }

    void ServerConnection::onSocketError(QAbstractSocket::SocketError error) {
        qDebug() << "socket error" << error;

        switch (_state) {
        case NotConnected:
            break; /* ignore error */
        case Connecting:
        case Handshake:
        case HandshakeFailure: /* just in case this one here too */
            emit cannotConnect(error);
            reset();
            break;
        case InOperation:
            _state = NotConnected;
            emit connectionBroken(error);
            reset();
            break;
        }
    }

    void ServerConnection::executeTextCommand(QString const& commandText) {
        if (commandText.startsWith("position ")) {
            QString positionText = commandText.mid(9);
            bool ok = false;
            qulonglong position = positionText.toULongLong(&ok);
            if (ok) {
                emit trackPositionChanged(position);
            }
        }
        else if (commandText == "playing") {
            emit playing();
        }
        else if (commandText == "paused") {
            emit paused();
        }
        else if (commandText == "stopped") {
            emit stopped();
        }
        else if (commandText.startsWith("volume ")) {

        }
        else if (commandText.startsWith("nowplaying ")) {
            QString rest = commandText.mid(11);

            if (rest == "nothing") {
                emit noCurrentTrack();
            }
            else if (rest.startsWith("track")) {
                QStringList list = rest.split('\n');

                QString title = "";
                QString artist = "";
                int length = -1;

                for (int i = 1; i < list.count(); ++i) {
                    QString line = list[i];
                    if (line.startsWith(" title: ")) {
                        title = line.mid(8);
                    }
                    else if (line.startsWith(" artist: ")) {
                        artist = line.mid(9);
                    }
                    else if (line.startsWith(" length: ")) {
                        QString lengthText = line.mid(9);
                        int spaceIndex = lengthText.indexOf(" sec");
                        if (spaceIndex > 0) {
                            lengthText = lengthText.mid(0, spaceIndex);

                            bool ok = false;
                            uint lengthUnsigned = lengthText.toUInt(&ok);
                            if (ok) { length = lengthUnsigned; }
                        }
                    }
                }

                emit nowPlayingTrack(title, artist, length);
            }
            else {
                qDebug() << "command not correctly formed:" << commandText;
            }
        }
        else {
            qDebug() << "command not handled:" << commandText;
        }
    }

    void ServerConnection::requestInitialInfo() {
        sendTextCommand("state"); /* request player state */
        sendTextCommand("nowplaying"); /* request track now playing */
        sendTextCommand("volume"); /* request current volume */
    }

    void ServerConnection::sendTextCommand(QString const& command) {
        qDebug() << "sending command" << command;
        _socket.write((command + ";").toUtf8());
    }

    void ServerConnection::shutdownServer() {
        sendTextCommand("shutdown");
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
