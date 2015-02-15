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

#include "serverconnection.h"

#include "common/networkutil.h"

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
        _binarySendingMode = false;
        _serverProtocolNo = -1;
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

                /* immediately switch to binary mode */
                sendTextCommand("binary");

                /* send binary hello */
                char binaryHeader[5];
                binaryHeader[0] = 'P';
                binaryHeader[1] = 'M';
                binaryHeader[2] = 'P';
                binaryHeader[3] = 0; /* protocol number; high byte */
                binaryHeader[4] = 1; /* protocol number; low byte */
                _socket.write(binaryHeader, sizeof(binaryHeader));

                _binarySendingMode = true;
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

                // FIXME: these request must become optional  (put them somewhere else)
                sendServerInstanceIdentifierRequest();
                requestDynamicModeStatus();

                emit connected();
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
        if (commandText == "binary") {
            /* switch to binary communication mode */
            _state = BinaryHandshake;
        }
        else {
            qDebug() << "ignoring text command:" << commandText;
        }
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
        NetworkUtil::append2Bytes(message, 1); /* message type */
        NetworkUtil::appendByte(message, action); /* single byte action type */

        sendBinaryMessage(message);
    }

    void ServerConnection::sendQueueFetchRequest(uint startOffset, quint8 length) {
        qDebug() << "sending queue fetch request, startOffset=" << startOffset << " length=" << (int)length;

        QByteArray message;
        message.reserve(7);
        NetworkUtil::append2Bytes(message, 4); /* message type */
        NetworkUtil::append4Bytes(message, startOffset);
        NetworkUtil::appendByte(message, length);

        sendBinaryMessage(message);
    }

    void ServerConnection::deleteQueueEntry(uint queueID) {
        QByteArray message;
        message.reserve(6);
        NetworkUtil::append2Bytes(message, 5); /* message type */
        NetworkUtil::append4Bytes(message, queueID);

        sendBinaryMessage(message);
    }

    void ServerConnection::sendTrackInfoRequest(uint queueID) {
        qDebug() << "sending request for track info of QID" << queueID;

        QByteArray message;
        message.reserve(6);
        NetworkUtil::append2Bytes(message, 2); /* message type */
        NetworkUtil::append4Bytes(message, queueID);

        sendBinaryMessage(message);
    }

    void ServerConnection::sendTrackInfoRequest(QList<uint> const& queueIDs) {
        qDebug() << "sending bulk request for track info of" << queueIDs.size() << "QIDs";

        QByteArray message;
        message.reserve(2 + 4 * queueIDs.size());
        NetworkUtil::append2Bytes(message, 3); /* message type */

        uint QID;
        foreach(QID, queueIDs) {
            NetworkUtil::append4Bytes(message, QID);
        }

        sendBinaryMessage(message);
    }

    void ServerConnection::sendPossibleFilenamesRequest(uint queueID) {
        qDebug() << "sending request for possible filenames of QID" << queueID;

        QByteArray message;
        message.reserve(6);
        NetworkUtil::append2Bytes(message, 7); /* message type */
        NetworkUtil::append4Bytes(message, queueID);

        sendBinaryMessage(message);
    }

    void ServerConnection::shutdownServer() {
        if (!_binarySendingMode) {
            return; /* too early for that */
        }

        sendSingleByteAction(99); /* 99 = shutdown server */
    }

    void ServerConnection::sendServerInstanceIdentifierRequest() {
        if (!_binarySendingMode) {
            return; /* too early for that */
        }

        sendSingleByteAction(12); /* 12 = request for server instance UUID */
    }

    void ServerConnection::requestPlayerState() {
        if (!_binarySendingMode) {
            return; /* too early for that */
        }

        sendSingleByteAction(10); /* 10 = request player state */
    }

    void ServerConnection::play() {
        if (!_binarySendingMode) {
            return; /* too early for that */
        }

        sendSingleByteAction(1); /* 1 = play */
    }

    void ServerConnection::pause() {
        if (!_binarySendingMode) {
            return; /* too early for that */
        }

        sendSingleByteAction(2); /* 2 = pause */
    }

    void ServerConnection::skip() {
        if (!_binarySendingMode) {
            return; /* too early for that */
        }

        sendSingleByteAction(3); /* 3 = skip */
    }

    void ServerConnection::seekTo(uint queueID, qint64 position) {
        if (!_binarySendingMode) {
            return; /* too early for that */
        }

        QByteArray message;
        message.reserve(14);
        NetworkUtil::append2Bytes(message, 8); /* message type */
        NetworkUtil::append4Bytes(message, queueID);
        NetworkUtil::append8Bytes(message, position);

        sendBinaryMessage(message);
    }

    void ServerConnection::setVolume(int percentage) {
        if (!_binarySendingMode) {
            return; /* too early for that */
        }

        sendSingleByteAction(100 + percentage); /* 100 to 200 = set volume */
    }

    void ServerConnection::enableDynamicMode() {
        if (!_binarySendingMode) {
            return; /* only supported in binary mode */
        }

        sendSingleByteAction(20); /* 20 = enable dynamic mode */
    }

    void ServerConnection::disableDynamicMode() {
        if (!_binarySendingMode) {
            return; /* only supported in binary mode */
        }

        sendSingleByteAction(21); /* 21 = disable dynamic mode */
    }

    void ServerConnection::expandQueue() {
        if (!_binarySendingMode) {
            return; /* only supported in binary mode */
        }

        sendSingleByteAction(22); /* 22 = request queue expansion */
    }

    void ServerConnection::requestDynamicModeStatus() {
        if (!_binarySendingMode) {
            return; /* only supported in binary mode */
        }

        sendSingleByteAction(11); /* 11 = request status of dynamic mode */
    }

    void ServerConnection::setDynamicModeNoRepetitionSpan(int seconds) {
        if (!_binarySendingMode) {
            return; /* only supported in binary mode */
        }

        QByteArray message;
        message.reserve(6);
        NetworkUtil::append2Bytes(message, 6); /* message type */
        NetworkUtil::append4Bytes(message, seconds);

        sendBinaryMessage(message);
    }

    void ServerConnection::readBinaryCommands() {
        char lengthBytes[4];

        while (_socket.peek(lengthBytes, sizeof(lengthBytes)) == sizeof(lengthBytes)) {
            quint32 messageLength = NetworkUtil::get4Bytes(lengthBytes);

            if (_socket.bytesAvailable() - sizeof(lengthBytes) < messageLength) {
                qDebug() << "waiting for incoming message with length" << messageLength << " --- only partially received";
                break; /* message not complete yet */
            }

            //qDebug() << "received complete binary message with length" << messageLength;

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

        int messageType = NetworkUtil::get2Bytes(message, 0);

        switch (messageType) {
        case 1: /* player state message */
        {
            if (messageLength != 20) {
                return; /* invalid message */
            }

            quint8 playerState = NetworkUtil::getByte(message, 2);
            quint8 volume = NetworkUtil::getByte(message, 3);
            quint32 queueLength = NetworkUtil::get4Bytes(message, 4);
            quint32 queueID = NetworkUtil::get4Bytes(message, 8);
            quint64 position = NetworkUtil::get8Bytes(message, 12);

            qDebug() << "received player state message";

            /* FIXME: events too simplistic */

            if (volume <= 100) { emit volumeChanged(volume); }

            if (queueID > 0) {
                emit nowPlayingTrack(queueID);
            }
            else {
                emit noCurrentTrack();
            }

            PlayState s = UnknownState;
            switch (playerState) {
            case 1:
                s = Stopped;
                emit stopped();
                break;
            case 2:
                s = Playing;
                emit playing();
                break;
            case 3:
                s = Paused;
                emit paused();
                break;
            }

            emit trackPositionChanged(position);
            emit queueLengthChanged(queueLength);
            emit receivedPlayerState(s, volume, queueLength, queueID, position);
        }
            break;
        case 2: /* volume change message */
        {
            if (messageLength != 3) {
                return; /* invalid message */
            }

            quint8 volume = NetworkUtil::getByte(message, 2);

            qDebug() << "received volume changed event;  volume:" << volume;

            if (volume <= 100) { emit volumeChanged(volume); }
        }
            break;
        case 3: /* track info reply message */
        {
            if (messageLength < 18) {
                return; /* invalid message */
            }

            quint32 queueID = NetworkUtil::get4Bytes(message, 2);
            qint32 lengthSeconds = NetworkUtil::get4Bytes(message, 6);
            quint32 titleSize = NetworkUtil::get4Bytes(message, 10);
            quint32 artistSize = NetworkUtil::get4Bytes(message, 14);

            if (queueID == 0) {
                return; /* invalid message */
            }

            if ((quint32)messageLength != 18 + titleSize + artistSize) {
                return; /* invalid message */
            }

            QString title = NetworkUtil::getUtf8String(message, 18, titleSize);
            QString artist = NetworkUtil::getUtf8String(message, 18 + titleSize, artistSize);

            qDebug() << "received track info reply;  QID:" << queueID << " seconds:" << lengthSeconds << " title:" << title << " artist:" << artist;

            receivedTrackInfo(queueID, lengthSeconds, title, artist);
        }
            break;
        case 4: /* bulk track info reply message */
        {
            if (messageLength < 18) {
                return; /* invalid message */
            }

            QList<int> offsets;
            offsets.append(2);
            int offset = 2;

            while (true) {
                quint32 queueID = NetworkUtil::get4Bytes(message, offset);
                //qint32 lengthSeconds = NetworkUtil::get4Bytes(message, offset + 4);
                quint32 titleSize = NetworkUtil::get4Bytes(message, offset + 8);
                quint32 artistSize = NetworkUtil::get4Bytes(message, offset + 12);
                int titleArtistOffset = offset + 16;

                if (queueID == 0) {
                    return; /* invalid message */
                }

                if (titleSize > (uint)messageLength - (uint)titleArtistOffset
                    || artistSize > (uint)messageLength - (uint)titleArtistOffset
                    || (titleSize + artistSize) > (uint)messageLength - (uint)titleArtistOffset)
                {
                    return; /* invalid message */
                }

                if (titleArtistOffset + titleSize + artistSize == (uint)messageLength) {
                    break; /* end of message */
                }

                /* at least one more track info follows */

                offset = offset + 16 + titleSize + artistSize;
                if (offset + 16 > messageLength) {
                    return;  /* invalid message */
                }

                offsets.append(offset);
            }

            qDebug() << "received bulk track info reply;  count:" << offsets.size();

            /* now read all track info's */
            foreach(offset, offsets) {
                quint32 queueID = NetworkUtil::get4Bytes(message, offset);
                qint32 lengthSeconds = NetworkUtil::get4Bytes(message, offset + 4);
                quint32 titleSize = NetworkUtil::get4Bytes(message, offset + 8);
                quint32 artistSize = NetworkUtil::get4Bytes(message, offset + 12);

                QString title = NetworkUtil::getUtf8String(message, offset + 16, titleSize);
                QString artist = NetworkUtil::getUtf8String(message, offset + 16 + titleSize, artistSize);

                emit receivedTrackInfo(queueID, lengthSeconds, title, artist);
            }
        }
            break;
        case 5: /* queue info reply message */
        {
            if (messageLength < 14) {
                return; /* invalid message */
            }

            quint32 queueLength = NetworkUtil::get4Bytes(message, 2);
            quint32 startOffset = NetworkUtil::get4Bytes(message, 6);

            QList<quint32> queueIDs;
            queueIDs.reserve((messageLength - 10) / 4);

            for (int offset = 10; offset < messageLength; offset += 4) {
                queueIDs.append(NetworkUtil::get4Bytes(message, offset));
            }

            if (queueLength - queueIDs.size() < startOffset) {
                return; /* invalid message */
            }

            qDebug() << "received queue contents;  Q-length:" << queueLength << " offset:" << startOffset << " count:" << queueIDs.size();

            emit receivedQueueContents(queueLength, startOffset, queueIDs);
        }
            break;
        case 6: /* queue entry removed */
        {
            if (messageLength != 10) {
                return; /* invalid message */
            }

            quint32 offset = NetworkUtil::get4Bytes(message, 2);
            quint32 queueID = NetworkUtil::get4Bytes(message, 6);

            qDebug() << "received queue track removal event;  QID:" << queueID << " offset:" << offset;

            emit queueEntryRemoved(offset, queueID);
        }
            break;
        case 7: /* queue entry added */
        {
            if (messageLength != 10) {
                return; /* invalid message */
            }

            quint32 offset = NetworkUtil::get4Bytes(message, 2);
            quint32 queueID = NetworkUtil::get4Bytes(message, 6);

            qDebug() << "received queue track insertion event;  QID:" << queueID << " offset:" << offset;

            emit queueEntryAdded(offset, queueID);
        }
            break;
        case 8: /* dynamic mode status */
        {
            if (messageLength != 7) return; /* invalid message */

            quint8 isEnabled = NetworkUtil::getByte(message, 2);
            int noRepetitionSpan = (int)NetworkUtil::get4Bytes(message, 3);

            if (noRepetitionSpan < 0) return; /* invalid message */

            qDebug() << "received dynamic mode status:" << (isEnabled > 0 ? "ON" : "OFF");

            emit dynamicModeStatusReceived(isEnabled > 0, noRepetitionSpan);
        }
            break;
        case 9: /* possible filenames for a QID */
        {
            if (messageLength < 6) return; /* invalid message */

            quint32 queueID = NetworkUtil::get4Bytes(message, 2);

            QList<QString> names;
            int offset = 6;
            while (offset < messageLength) {
                if (offset > messageLength - 4) return; /* invalid message */
                quint32 nameLength = NetworkUtil::get4Bytes(message, offset);
                offset += 4;
                if (nameLength + (uint)offset > (uint)messageLength) return; /* invalid message */
                QString name = NetworkUtil::getUtf8String(message, offset, nameLength);
                offset += nameLength;
                names.append(name);
            }

            qDebug() << "received a list of" << names.size() << "possible filenames for QID" << queueID;

            if (names.size() == 1) {
                qDebug() << " received name" << names[0];
            }

            emit receivedPossibleFilenames(queueID, names);
        }
            break;
        case 10: /* server instance identifier response */
        {
            if (messageLength != (2 + 16)) return; /* invalid message */

            QUuid uuid = QUuid::fromRfc4122(message.mid(2));
            qDebug() << "received server instance identifier:" << uuid;

            emit receivedServerInstanceIdentifier(uuid);
        }
            break;
        default:
            qDebug() << "received unknown binary message type" << messageType << " with length" << messageLength;
            break; /* unknown message type */
        }
    }

}
