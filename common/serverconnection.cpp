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
     : QObject(parent),
      _state(ServerConnection::NotConnected),
      _binarySendingMode(false),
      _serverProtocolNo(-1)
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
        const bool START_IN_BINARY_MODE = true;

        State state;
        do {
            state = _state;

            switch (state) {
            case NotConnected:
                break; /* problem */
            case Connecting: /* fall-through */
            case Handshake:
            {
                char heading[3];

                if (_socket.peek(heading, 3) < 3) {
                    /* not enough data */
                    break;
                }

                if (heading[0] != 'P'
                    || heading[1] != 'M'
                    || heading[2] != 'P')
                {
                    _state = HandshakeFailure;
                    emit invalidServer();
                    reset();
                    return;
                }

                bool hadSemicolon = false;
                char c;
                while (_socket.getChar(&c)) {
                    if (c == ';') {
                        hadSemicolon = true;
                        break;
                    }
                    _readBuffer.append(c);
                }

                if (!hadSemicolon) break; /* not enough data yet */

                QString serverHelloString = QString::fromUtf8(_readBuffer);
                _readBuffer.clear(); /* text consumed */

                qDebug() << "server hello:" << serverHelloString;

                /* TODO: other checks */

                /* TODO: maybe extract server version */

                _state = TextMode;

                if (START_IN_BINARY_MODE) {
                    switchToBinaryMode(); // EXPERIMENTAL
                }
                else {
                    requestInitialInfo();
                    emit connected();
                }
            }
                break;
            case TextMode:
                readTextCommands();
                break;
            case BinaryHandshake:
            {
                if (_socket.bytesAvailable() < 5) {
                    /* not enough data */
                    break;
                }

                char heading[5];
                _socket.read(heading, 5);

                if (heading[0] != 'P'
                    || heading[1] != 'M'
                    || heading[2] != 'P')
                {
                    _state = HandshakeFailure;
                    emit invalidServer();
                    reset();
                    return;
                }

                _serverProtocolNo = (heading[3] << 8) + heading[4];
                qDebug() << "server supports protocol " << QString::number(_serverProtocolNo);

                _state = BinaryMode;

                if (START_IN_BINARY_MODE) {
                    emit connected();
                    sendSingleByteAction(10); /* request player state */
                }
            }
                break;
            case BinaryMode:
                readBinaryCommands();
                break;
            case HandshakeFailure:
                /* do nothing */
                break;
            }

            /* maybe a new state will consume the available bytes... */
        } while (_state != state && _socket.bytesAvailable() > 0);
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
        case TextMode:
        case BinaryHandshake:
        case BinaryMode:
            _state = NotConnected;
            emit connectionBroken(error);
            reset();
            break;
        }
    }

    void ServerConnection::readTextCommands() {
        do {
            bool hadSemicolon = false;
            char c;
            while (_socket.getChar(&c)) {
                if (c == ';') {
                    hadSemicolon = true;
                    break;
                }
                _readBuffer.append(c);
            }

            if (!hadSemicolon) break; /* not enough data yet */

            QString commandString = QString::fromUtf8(_readBuffer);
            _readBuffer.clear();

            executeTextCommand(commandString);
        } while (_state == TextMode);
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
            QString volumeText = commandText.mid(7);
            bool ok = false;
            quint8 volume = volumeText.toUInt(&ok);
            if (ok) {
                emit volumeChanged(volume);
            }
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
                qint64 position = -1;

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
                    else if (line.startsWith(" position: ")) {
                        QString positionText = line.mid(11);
                        bool ok = false;
                        qulonglong value = positionText.toULongLong(&ok);
                        if (ok) {
                            qDebug() << "valid position in nowplaying:" << value;
                            position = value;
                        }
                    }
                }

                emit nowPlayingTrack(title, artist, length);
                if (position >= 0) emit trackPositionChanged(position);
            }
            else {
                qDebug() << "command not correctly formed:" << commandText;
            }
        }
        else if (commandText == "binary") {
            /* switch to binary communication mode */
            _state = BinaryHandshake;
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

    void ServerConnection::switchToBinaryMode() {
        if (_state != TextMode) return;

        sendTextCommand("binary");

        char binaryHeader[5];
        binaryHeader[0] = 'P';
        binaryHeader[1] = 'M';
        binaryHeader[2] = 'P';
        binaryHeader[3] = 0; /* protocol number; high byte */
        binaryHeader[4] = 1; /* protocol number; low byte */
        _socket.write(binaryHeader, sizeof(binaryHeader));

        _binarySendingMode = true;
    }

    void ServerConnection::sendTextCommand(QString const& command) {
        qDebug() << "sending command" << command;
        _socket.write((command + ";").toUtf8());
    }

    void ServerConnection::sendBinaryMessage(QByteArray const& message) {
        quint32 length = message.length();

        char lengthArr[4];
        lengthArr[0] = (length >> 24) & 255;
        lengthArr[1] = (length >> 16) & 255;
        lengthArr[2] = (length >> 8) & 255;
        lengthArr[3] = length & 255;

        _socket.write(lengthArr, sizeof(lengthArr));
        _socket.write(message.data(), length);
    }

    void ServerConnection::sendSingleByteAction(quint8 action) {
        qDebug() << "sending single byte action" << (int)action;

        QByteArray message;
        message.reserve(3);
        message.append((char)0); /* message type, high byte */
        message.append((char)1); /* message type, low byte */
        message.append((char)action); /* single byte action type */

        sendBinaryMessage(message);
    }

    void ServerConnection::shutdownServer() {
        if (!_binarySendingMode) {
            sendTextCommand("shutdown");
            return;
        }

        sendSingleByteAction(99); /* 99 = shutdown server */
    }

    void ServerConnection::play() {
        if (!_binarySendingMode) {
            sendTextCommand("play");
            return;
        }

        sendSingleByteAction(1); /* 1 = play */
    }

    void ServerConnection::pause() {
        if (!_binarySendingMode) {
            sendTextCommand("pause");
            return;
        }

        sendSingleByteAction(2); /* 2 = pause */
    }

    void ServerConnection::skip() {
        if (!_binarySendingMode) {
            sendTextCommand("skip");
            return;
        }

        sendSingleByteAction(3); /* 3 = skip */
    }

    void ServerConnection::setVolume(int percentage) {
        if (!_binarySendingMode) {
            sendTextCommand("volume " + QString::number(percentage));
            return;
        }

        sendSingleByteAction(100 + percentage); /* 100 to 200 = set volume */
    }

    void ServerConnection::readBinaryCommands() {
        char lengthBytes[4];

        while (_socket.peek(lengthBytes, sizeof(lengthBytes)) == sizeof(lengthBytes)) {
            quint32 messageLength =
                (lengthBytes[0] << 24)
                + (lengthBytes[1] << 16)
                + (lengthBytes[2] << 8)
                + lengthBytes[3];

            qDebug() << "binary message length:" << messageLength;

            if (_socket.bytesAvailable() - sizeof(lengthBytes) < messageLength) {
                break; /* message not complete yet */
            }

            _socket.read(lengthBytes, sizeof(lengthBytes)); /* consume the length */
            QByteArray message = _socket.read(messageLength);

            handleBinaryMessage(message);
        }
    }

    void ServerConnection::handleBinaryMessage(QByteArray const& message) {
        qint32 messageLength = message.length();

        if (messageLength < 2) {
            qDebug() << "received invalid binary message (less than 2 bytes)";
            return; /* invalid message */
        }

        int messageType = (message[0] << 8) + message[1];
        qDebug() << "received binary message with type" << messageType;

        switch (messageType) {
        case 1:
        {
            if (messageLength != 20) {
                return; /* invalid message */
            }

            quint8 playerState = message[2];
            quint8 volume = message[3];

//            quint32 queueLength =
//                (message[4] << 24)
//                + (message[5] << 16)
//                + (message[6] << 8)
//                + message[7];
//
//            quint32 queueID =
//                (message[8] << 24)
//                + (message[9] << 16)
//                + (message[10] << 8)
//                + message[11];

            quint64 position = message[12];
            position = (position << 8) + message[13];
            position = (position << 8) + message[14];
            position = (position << 8) + message[15];
            position = (position << 8) + message[16];
            position = (position << 8) + message[17];
            position = (position << 8) + message[18];
            position = (position << 8) + message[19];

            /* FIXME: events too simplistic */

            if (volume <= 100) { emit volumeChanged(volume); }

            switch (playerState) {
            case 1:
                emit stopped();
                break;
            case 2:
                emit playing();
                break;
            case 3:
                emit paused();
                break;
            }

            emit trackPositionChanged(position);


        }
            break;
        case 2:
        {
            if (messageLength != 3) {
                return; /* invalid message */
            }

            quint8 volume = message[2];

            if (volume <= 100) { emit volumeChanged(volume); }
        }
            break;
        default:
            qDebug() << "unknown binary message type" << messageType;
            break; /* unknown message type */
        }
    }

}
