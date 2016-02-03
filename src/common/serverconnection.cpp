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

#include "serverconnection.h"

#include "common/networkprotocol.h"
#include "common/networkutil.h"

#include <QtDebug>

namespace PMP {

    ServerConnection::ServerConnection(QObject* parent)
     : QObject(parent),
      _state(ServerConnection::NotConnected),
      _binarySendingMode(false),
      _serverProtocolNo(-1), _nextRef(1),
      _userAccountRegistrationRef(0), _userLoginRef(0), _userLoggedInId(0)
    {
        connect(&_socket, SIGNAL(connected()), this, SLOT(onConnected()));
        connect(&_socket, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
        connect(
            &_socket, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(onSocketError(QAbstractSocket::SocketError))
        );
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

    quint32 ServerConnection::userLoggedInId() const {
        return _userLoggedInId;
    }

    QString ServerConnection::userLoggedInName() const {
        return _userLoggedInName;
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
        if (!_binarySendingMode) {
            qDebug() << "PROBLEM: cannot send single byte action yet; action:"
                     << (int)action;
            return; /* too early for that */
        }

        qDebug() << "sending single byte action" << (int)action;

        QByteArray message;
        message.reserve(3);
        NetworkUtil::append2Bytes(message, NetworkProtocol::SingleByteActionMessage);
        NetworkUtil::appendByte(message, action); /* single byte action type */

        sendBinaryMessage(message);
    }

    void ServerConnection::sendQueueFetchRequest(uint startOffset, quint8 length) {
        qDebug() << "sending queue fetch request, startOffset=" << startOffset << " length=" << (int)length;

        QByteArray message;
        message.reserve(7);
        NetworkUtil::append2Bytes(message, NetworkProtocol::QueueFetchRequestMessage);
        NetworkUtil::append4Bytes(message, startOffset);
        NetworkUtil::appendByte(message, length);

        sendBinaryMessage(message);
    }

    void ServerConnection::deleteQueueEntry(uint queueID) {
        QByteArray message;
        message.reserve(6);
        NetworkUtil::append2Bytes(message, NetworkProtocol::QueueEntryRemovalRequestMessage);
        NetworkUtil::append4Bytes(message, queueID);

        sendBinaryMessage(message);
    }

    void ServerConnection::moveQueueEntry(uint queueID, qint16 offsetDiff) {
        QByteArray message;
        message.reserve(8);
        NetworkUtil::append2Bytes(message, NetworkProtocol::QueueEntryMoveRequestMessage);
        NetworkUtil::append2Bytes(message, offsetDiff);
        NetworkUtil::append4Bytes(message, queueID);

        sendBinaryMessage(message);
    }

    void ServerConnection::insertQueueEntryAtFront(FileHash const& hash) {
        QByteArray message;
        message.reserve(2 + 2 + NetworkProtocol::FILEHASH_BYTECOUNT);
        NetworkUtil::append2Bytes(message, NetworkProtocol::AddHashToFrontOfQueueRequestMessage);
        NetworkUtil::append2Bytes(message, 0); /* filler */
        NetworkProtocol::appendHash(message, hash);

        sendBinaryMessage(message);
    }

    void ServerConnection::insertQueueEntryAtEnd(FileHash const& hash) {
        QByteArray message;
        message.reserve(2 + 2 + NetworkProtocol::FILEHASH_BYTECOUNT);
        NetworkUtil::append2Bytes(message, NetworkProtocol::AddHashToEndOfQueueRequestMessage);
        NetworkUtil::append2Bytes(message, 0); /* filler */
        NetworkProtocol::appendHash(message, hash);

        sendBinaryMessage(message);
    }

    void ServerConnection::sendTrackInfoRequest(uint queueID) {
        if (queueID == 0) return;

        qDebug() << "sending request for track info of QID" << queueID;

        QByteArray message;
        message.reserve(6);
        NetworkUtil::append2Bytes(message, NetworkProtocol::TrackInfoRequestMessage);
        NetworkUtil::append4Bytes(message, queueID);

        sendBinaryMessage(message);
    }

    void ServerConnection::sendTrackInfoRequest(QList<uint> const& queueIDs) {
        if (queueIDs.empty()) return;

        qDebug() << "sending bulk request for track info of" << queueIDs.size() << "QIDs";

        QByteArray message;
        message.reserve(2 + 4 * queueIDs.size());
        NetworkUtil::append2Bytes(message, NetworkProtocol::BulkTrackInfoRequestMessage);

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
        NetworkUtil::append2Bytes(message, NetworkProtocol::PossibleFilenamesForQueueEntryRequestMessage);
        NetworkUtil::append4Bytes(message, queueID);

        sendBinaryMessage(message);
    }

    void ServerConnection::createNewUserAccount(QString login, QString password) {
        _userAccountRegistrationRef = _nextRef;
        _nextRef++;

        _userAccountRegistrationLogin = login;
        _userAccountRegistrationPassword = password;

        sendInitiateNewUserAccountMessage(login, _userAccountRegistrationRef);
    }

    void ServerConnection::login(QString login, QString password) {
        _userLoginRef = _nextRef;
        _nextRef++;

        _userLoggingIn = login;
        _userLoggingInPassword = password;

        sendInitiateLoginMessage(login, _userLoginRef);
    }

    void ServerConnection::switchToPublicMode() {
        sendSingleByteAction(30);
    }

    void ServerConnection::switchToPersonalMode() {
        sendSingleByteAction(31);
    }

    void ServerConnection::requestUserPlayingForMode() {
        sendSingleByteAction(14);
    }

    void ServerConnection::handleNewUserSalt(QString login, QByteArray salt) {
        if (login != _userAccountRegistrationLogin) return;

        uint reference = _userAccountRegistrationRef;

        QByteArray hashedPassword =
            NetworkProtocol::hashPassword(salt, _userAccountRegistrationPassword);

        sendFinishNewUserAccountMessage(login, salt, hashedPassword, reference);
    }

    void ServerConnection::handleLoginSalt(QString login, QByteArray userSalt,
                                           QByteArray sessionSalt)
    {
        if (login != _userLoggingIn) return;

        uint reference = _userLoginRef;

        QByteArray hashedPassword =
            NetworkProtocol::hashPasswordForSession(
                userSalt, sessionSalt, _userLoggingInPassword
            );

        sendFinishLoginMessage(login, userSalt, sessionSalt, hashedPassword, reference);
    }

    void ServerConnection::handleUserRegistrationResult(quint16 errorType,
                                                        quint32 intData,
                                                        QByteArray const& blobData)
    {
        (void)blobData; /* we don't use this */

        QString login = _userAccountRegistrationLogin;

        /* clean up potentially sensitive information */
        _userAccountRegistrationLogin = "";
        _userAccountRegistrationPassword = "";

        if (errorType == 0) {
            emit userAccountCreatedSuccessfully(login, intData);
        }
        else {
            UserRegistrationError error;

            switch (errorType) {
            case NetworkProtocol::UserAccountAlreadyExists:
                error = AccountAlreadyExists;
                break;
            case NetworkProtocol::InvalidUserAccountName:
                error = InvalidAccountName;
                break;
            default:
                error = UnknownUserRegistrationError;
                break;
            }

            emit userAccountCreationError(login, error);
        }
    }

    void ServerConnection::handleUserLoginResult(quint16 errorType, quint32 intData,
                                                 const QByteArray &blobData)
    {
        (void)blobData; /* we don't use this */

        QString login = _userLoggingIn;
        quint32 userId = intData;

        qDebug() << " received login result: errorType =" << errorType
                 << "; login =" << login << "; id =" << userId;

        /* clean up potentially sensitive information */
        _userLoggingInPassword = "";

        if (errorType == 0) {
            _userLoggedInId = userId;
            _userLoggedInName = _userLoggingIn;
            _userLoggingIn = "";
            emit userLoggedInSuccessfully(login, intData);
        }
        else {
            _userLoggingIn = "";

            UserLoginError error;

            switch (errorType) {
            case NetworkProtocol::UserLoginAuthenticationFailed:
                error = UserLoginAuthenticationFailed;
                break;
            default:
                error = UnknownUserLoginError;
                break;
            }

            emit userLoginError(login, error);
        }
    }

    void ServerConnection::onFullIndexationRunningStatusReceived(bool running) {
        auto oldValue = _doingFullIndexation;
        _doingFullIndexation = running;

        emit fullIndexationStatusReceived(running);

        if (oldValue.isKnown()) {
            if (running)
                emit fullIndexationStarted();
            else
                emit fullIndexationFinished();
        }
    }

    void ServerConnection::sendUserAccountsFetchRequest() {
        sendSingleByteAction(13); /* 13 = fetch list of user accounts */
    }

    void ServerConnection::shutdownServer() {
        sendSingleByteAction(99); /* 99 = shutdown server */
    }

    void ServerConnection::sendServerInstanceIdentifierRequest() {
        sendSingleByteAction(12); /* 12 = request for server instance UUID */
    }

    void ServerConnection::sendServerNameRequest() {
        sendSingleByteAction(16); /* 16 = request for server name */
    }

    void ServerConnection::requestPlayerState() {
        sendSingleByteAction(10); /* 10 = request player state */
    }

    void ServerConnection::play() {
        sendSingleByteAction(1); /* 1 = play */
    }

    void ServerConnection::pause() {
        sendSingleByteAction(2); /* 2 = pause */
    }

    void ServerConnection::skip() {
        sendSingleByteAction(3); /* 3 = skip */
    }

    void ServerConnection::insertPauseAtFront() {
        sendSingleByteAction(4); /* 4 = insert pause at front */
    }

    void ServerConnection::seekTo(uint queueID, qint64 position) {
        if (!_binarySendingMode) {
            return; /* too early for that */
        }

        QByteArray message;
        message.reserve(14);
        NetworkUtil::append2Bytes(message, NetworkProtocol::PlayerSeekRequestMessage);
        NetworkUtil::append4Bytes(message, queueID);
        NetworkUtil::append8Bytes(message, position);

        sendBinaryMessage(message);
    }

    void ServerConnection::setVolume(int percentage) {
        sendSingleByteAction(100 + percentage); /* 100 to 200 = set volume */
    }

    void ServerConnection::enableDynamicMode() {
        sendSingleByteAction(20); /* 20 = enable dynamic mode */
    }

    void ServerConnection::disableDynamicMode() {
        sendSingleByteAction(21); /* 21 = disable dynamic mode */
    }

    void ServerConnection::expandQueue() {
        sendSingleByteAction(22); /* 22 = request queue expansion */
    }

    void ServerConnection::trimQueue() {
        sendSingleByteAction(23); /* 23 = request queue trim */
    }

    void ServerConnection::requestDynamicModeStatus() {
        sendSingleByteAction(11); /* 11 = request status of dynamic mode */
    }

    void ServerConnection::setDynamicModeNoRepetitionSpan(int seconds) {
        if (!_binarySendingMode) {
            return; /* only supported in binary mode */
        }

        QByteArray message;
        message.reserve(6);
        NetworkUtil::append2Bytes(message, NetworkProtocol::GeneratorNonRepetitionChangeMessage);
        NetworkUtil::append4Bytes(message, seconds);

        sendBinaryMessage(message);
    }

    void ServerConnection::startFullIndexation() {
        sendSingleByteAction(40); /* 40 = start full indexation */
    }

    void ServerConnection::requestFullIndexationRunningStatus() {
        sendSingleByteAction(15); /* 15 = request for full indexation running status */
    }

    void ServerConnection::fetchCollection(AbstractCollectionFetcher* fetcher) {
        auto fetcherReference = _nextRef;
        _nextRef++;

        _collectionFetchers[fetcherReference] = fetcher;
        sendCollectionFetchRequestMessage(fetcherReference);
    }

    void ServerConnection::sendInitiateNewUserAccountMessage(QString login,
                                                             quint32 clientReference)
    {
        if (!_binarySendingMode) {
            return; /* only supported in binary mode */
        }

        QByteArray loginBytes = login.toUtf8();

        QByteArray message;
        message.reserve(2 + 2 + 4 + loginBytes.size());
        NetworkUtil::append2Bytes(message, NetworkProtocol::InitiateNewUserAccountMessage);
        NetworkUtil::appendByte(message, loginBytes.size());
        NetworkUtil::appendByte(message, 0); /* unused */
        NetworkUtil::append4Bytes(message, clientReference);
        message += loginBytes;

        sendBinaryMessage(message);
    }

    void ServerConnection::sendInitiateLoginMessage(QString login,
                                                    quint32 clientReference)
    {
        if (!_binarySendingMode) {
            return; /* only supported in binary mode */
        }

        QByteArray loginBytes = login.toUtf8();

        QByteArray message;
        message.reserve(2 + 2 + 4 + loginBytes.size());
        NetworkUtil::append2Bytes(message, NetworkProtocol::InitiateLoginMessage);
        NetworkUtil::appendByte(message, loginBytes.size());
        NetworkUtil::appendByte(message, 0); /* unused */
        NetworkUtil::append4Bytes(message, clientReference);
        message += loginBytes;

        sendBinaryMessage(message);
    }

    void ServerConnection::sendFinishNewUserAccountMessage(QString login, QByteArray salt,
                                                           QByteArray hashedPassword,
                                                           quint32 clientReference)
    {
        if (!_binarySendingMode) {
            return; /* only supported in binary mode */
        }

        QByteArray loginBytes = login.toUtf8();

        QByteArray message;
        message.reserve(4 + 4 + loginBytes.size() + salt.size() + hashedPassword.size());
        NetworkUtil::append2Bytes(message, NetworkProtocol::FinishNewUserAccountMessage);
        NetworkUtil::appendByte(message, loginBytes.size());
        NetworkUtil::appendByte(message, salt.size());
        NetworkUtil::append4Bytes(message, clientReference);
        message += loginBytes;
        message += salt;
        message += hashedPassword;

        sendBinaryMessage(message);
    }

    void ServerConnection::sendFinishLoginMessage(QString login, QByteArray userSalt,
                                                  QByteArray sessionSalt,
                                                  QByteArray hashedPassword,
                                                  quint32 clientReference)
    {
        if (!_binarySendingMode) {
            return; /* only supported in binary mode */
        }

        QByteArray loginBytes = login.toUtf8();

        QByteArray message;
        message.reserve(
            4 + 4 + 4 + loginBytes.size() + userSalt.size() + sessionSalt.size()
            + hashedPassword.size()
        );
        NetworkUtil::append2Bytes(message, NetworkProtocol::FinishLoginMessage);
        NetworkUtil::append2Bytes(message, 0); /* unused */
        NetworkUtil::appendByte(message, (quint8)loginBytes.size());
        NetworkUtil::appendByte(message, (quint8)userSalt.size());
        NetworkUtil::appendByte(message, (quint8)sessionSalt.size());
        NetworkUtil::appendByte(message, (quint8)hashedPassword.size());
        NetworkUtil::append4Bytes(message, clientReference);
        message += loginBytes;
        message += userSalt;
        message += sessionSalt;
        message += hashedPassword;

        sendBinaryMessage(message);
    }

    void ServerConnection::sendCollectionFetchRequestMessage(uint clientReference) {
        if (!_binarySendingMode) {
            return; /* only supported in binary mode */
        }

        QByteArray message;
        message.reserve(4 + 4);
        NetworkUtil::append2Bytes(message, NetworkProtocol::CollectionFetchRequestMessage);
        NetworkUtil::append2Bytes(message, 0); /* unused */
        NetworkUtil::append4Bytes(message, clientReference);

        sendBinaryMessage(message);
    }

    void ServerConnection::readBinaryCommands() {
        char lengthBytes[4];

        while (_socket.peek(lengthBytes, sizeof(lengthBytes)) == sizeof(lengthBytes)) {
            quint32 messageLength = NetworkUtil::get4Bytes(lengthBytes);

            if (_socket.bytesAvailable() - sizeof(lengthBytes) < messageLength) {
                qDebug() << "waiting for incoming message with length" << messageLength
                         << " --- only partially received";
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

        auto messageType =
            (NetworkProtocol::ServerMessageType)NetworkUtil::get2Bytes(message, 0);

        switch (messageType) {
        case NetworkProtocol::ServerEventNotificationMessage:
        {
            if (messageLength != 4) {
                return; /* invalid message */
            }

            quint8 event = NetworkUtil::getByte(message, 2);
            quint8 eventArg = NetworkUtil::getByte(message, 3);

            qDebug() << "received server event" << event << "with arg" << eventArg;

            switch (event) {
            case 1:
                onFullIndexationRunningStatusReceived(true);
                break;
            case 2:
                onFullIndexationRunningStatusReceived(false);
                break;
            default:
                qDebug() << "received unknown server event:" << event;
                break;
            }
        }
            break;
        case NetworkProtocol::PlayerStateMessage:
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
        case NetworkProtocol::VolumeChangedMessage:
        {
            if (messageLength != 3) {
                return; /* invalid message */
            }

            quint8 volume = NetworkUtil::getByte(message, 2);

            qDebug() << "received volume changed event;  volume:" << volume;

            if (volume <= 100) { emit volumeChanged(volume); }
        }
            break;
        case NetworkProtocol::TrackInfoMessage:
        {
            if (messageLength < 16) {
                return; /* invalid message */
            }

            quint16 status = NetworkUtil::get2Bytes(message, 2);
            quint32 queueID = NetworkUtil::get4Bytes(message, 4);
            int lengthSeconds = (qint32)NetworkUtil::get4Bytes(message, 8);
            int titleSize = (uint)NetworkUtil::get2Bytes(message, 12);
            int artistSize = (uint)NetworkUtil::get2Bytes(message, 14);

            if (queueID == 0) {
                return; /* invalid message */
            }

            if (messageLength != 16 + titleSize + artistSize) {
                return; /* invalid message */
            }

            QString title, artist;
            if (NetworkProtocol::isTrackStatusFromRealTrack(status)) {
                title = NetworkUtil::getUtf8String(message, 16, titleSize);
                artist = NetworkUtil::getUtf8String(message, 16 + titleSize, artistSize);
            }
            else {
                title = artist = NetworkProtocol::getPseudoTrackStatusText(status);
            }

            qDebug() << "received track info reply;  QID:" << queueID
                << " seconds:" << lengthSeconds << " title:" << title
                << " artist:" << artist;

            receivedTrackInfo(queueID, lengthSeconds, title, artist);
        }
            break;
        case NetworkProtocol::BulkTrackInfoMessage:
        {
            if (messageLength < 4) {
                return; /* invalid message */
            }

            int trackCount = (uint)NetworkUtil::get2Bytes(message, 2);
            int statusBlockCount = trackCount + trackCount % 2;
            if (trackCount == 0
                || messageLength < 4 + statusBlockCount * 2 + trackCount * 12)
            {
                return; /* irrelevant or invalid message */
            }

            int offset = 4;
            QList<quint16> statuses;
            for (int i = 0; i < trackCount; ++i) {
                statuses.append(NetworkUtil::get2Bytes(message, offset));
                offset += 2;
            }
            if (trackCount % 2 != 0) {
                offset += 2; /* skip filler */
            }

            QList<int> offsets;
            offsets.append(offset);
            while (true) {
                quint32 queueID = NetworkUtil::get4Bytes(message, offset);
                //int lengthSeconds = (uint)NetworkUtil::get4Bytes(message, offset + 4);
                int titleSize = (uint)NetworkUtil::get2Bytes(message, offset + 8);
                int artistSize = (uint)NetworkUtil::get2Bytes(message, offset + 10);
                int titleArtistOffset = offset + 12;

                if (queueID == 0) {
                    return; /* invalid message */
                }

                if (titleSize > messageLength - titleArtistOffset
                    || artistSize > messageLength - titleArtistOffset
                    || (titleSize + artistSize) > messageLength - titleArtistOffset)
                {
                    return; /* invalid message */
                }

                if (titleArtistOffset + titleSize + artistSize == messageLength) {
                    break; /* end of message */
                }

                /* at least one more track info follows */

                offset = offset + 12 + titleSize + artistSize;
                if (offset + 12 > messageLength) {
                    return;  /* invalid message */
                }

                offsets.append(offset);
            }

            qDebug() << "received bulk track info reply;  count:" << trackCount;

            if (trackCount != offsets.size()) {
                return;  /* invalid message */
            }

            /* now read all track info's */
            for (int i = 0; i < trackCount; ++i) {
                offset = offsets[i];
                quint32 queueID = NetworkUtil::get4Bytes(message, offset);
                quint16 status = statuses[i];
                int lengthSeconds = (uint)NetworkUtil::get4Bytes(message, offset + 4);
                int titleSize = (uint)NetworkUtil::get2Bytes(message, offset + 8);
                int artistSize = (uint)NetworkUtil::get2Bytes(message, offset + 10);
                offset += 12;

                QString title, artist;
                if (NetworkProtocol::isTrackStatusFromRealTrack(status)) {
                    title = NetworkUtil::getUtf8String(message, offset, titleSize);
                    artist =
                        NetworkUtil::getUtf8String(
                            message, offset + titleSize, artistSize
                        );
                }
                else {
                    title = artist = NetworkProtocol::getPseudoTrackStatusText(status);
                }

                emit receivedTrackInfo(queueID, lengthSeconds, title, artist);
            }
        }
            break;
        case NetworkProtocol::QueueContentsMessage:
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

            qDebug() << "received queue contents;  Q-length:" << queueLength
                     << " offset:" << startOffset << " count:" << queueIDs.size();

            emit receivedQueueContents(queueLength, startOffset, queueIDs);
        }
            break;
        case NetworkProtocol::QueueEntryRemovedMessage:
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
        case NetworkProtocol::QueueEntryAddedMessage:
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
        case NetworkProtocol::DynamicModeStatusMessage:
        {
            if (messageLength != 7) return; /* invalid message */

            quint8 isEnabled = NetworkUtil::getByte(message, 2);
            int noRepetitionSpan = (int)NetworkUtil::get4Bytes(message, 3);

            if (noRepetitionSpan < 0) return; /* invalid message */

            qDebug() << "received dynamic mode status:" << (isEnabled > 0 ? "ON" : "OFF");

            emit dynamicModeStatusReceived(isEnabled > 0, noRepetitionSpan);
        }
            break;
        case NetworkProtocol::PossibleFilenamesForQueueEntryMessage:
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
        case NetworkProtocol::ServerInstanceIdentifierMessage:
        {
            if (messageLength != (2 + 16)) return; /* invalid message */

            QUuid uuid = QUuid::fromRfc4122(message.mid(2));
            qDebug() << "received server instance identifier:" << uuid;

            emit receivedServerInstanceIdentifier(uuid);
        }
            break;
        case NetworkProtocol::QueueEntryMovedMessage:
        {
            if (messageLength != 14) {
                return; /* invalid message */
            }

            quint32 fromOffset = NetworkUtil::get4Bytes(message, 2);
            quint32 toOffset = NetworkUtil::get4Bytes(message, 6);
            quint32 queueID = NetworkUtil::get4Bytes(message, 10);

            qDebug() << "received queue track moved event;  QID:" << queueID
                     << " from-offset:" << fromOffset << " to-offset:" << toOffset;

            emit queueEntryMoved(fromOffset, toOffset, queueID);
        }
            break;
        case NetworkProtocol::UsersListMessage:
        {
            if (messageLength < 4) {
                return; /* invalid message */
            }

            QList<QPair<uint, QString> > users;

            int userCount = (uint)NetworkUtil::get2Bytes(message, 2);

            qDebug() << "received user account list; count:" << userCount;
            qDebug() << " message length=" << messageLength;

            int offset = 4;
            for (int i = 0; i < userCount; ++i) {
                if (messageLength - offset < 5) {
                    return; /* invalid message */
                }

                quint32 userId = NetworkUtil::get4Bytes(message, offset);
                offset += 4;
                int loginNameByteCount = (uint)NetworkUtil::getByte(message, offset);
                offset += 1;
                if (messageLength - offset < loginNameByteCount) {
                    return; /* invalid message */
                }

                QString login =
                    NetworkUtil::getUtf8String(message, offset, loginNameByteCount);
                offset += loginNameByteCount;

                users.append(QPair<uint, QString>(userId, login));
            }

            if (offset != messageLength) {
                return; /* invalid message */
            }

            emit receivedUserAccounts(users);
        }
            break;
        case NetworkProtocol::NewUserAccountSaltMessage:
        {
            if (messageLength < 4) {
                return; /* invalid message */
            }

            int loginBytesSize = (uint)NetworkUtil::getByte(message, 2);
            int saltBytesSize = (uint)NetworkUtil::getByte(message, 3);

            if (messageLength != 4 + loginBytesSize + saltBytesSize) {
                return; /* invalid message */
            }

            qDebug() << "received salt for new user account";

            QString login = NetworkUtil::getUtf8String(message, 4, loginBytesSize);
            QByteArray salt = message.mid(4 + loginBytesSize, saltBytesSize);

            handleNewUserSalt(login, salt);
        }
            break;
        case NetworkProtocol::SimpleResultMessage:
        {
            if (messageLength < 12) {
                return; /* invalid message */
            }

            quint16 errorType = NetworkUtil::get2Bytes(message, 2);
            quint32 clientReference = NetworkUtil::get4Bytes(message, 4);
            quint32 intData = NetworkUtil::get4Bytes(message, 8);

            QByteArray blobData = message.mid(12);

            qDebug() << "received result/error message; type:" << errorType
                     << " client-ref:" << clientReference;

            handleResultMessage(errorType, clientReference, intData, blobData);
        }
            break;
        case NetworkProtocol::UserLoginSaltMessage:
        {
            if (messageLength < 8) {
                return; /* invalid message */
            }

            int loginBytesSize = (uint)NetworkUtil::getByte(message, 4);
            int userSaltBytesSize = (uint)NetworkUtil::getByte(message, 5);
            int sessionSaltBytesSize = (uint)NetworkUtil::getByte(message, 6);

            if (messageLength !=
                    8 + loginBytesSize + userSaltBytesSize + sessionSaltBytesSize)
            {
                return; /* invalid message */
            }

            QString login = NetworkUtil::getUtf8String(message, 8, loginBytesSize);
            QByteArray userSalt = message.mid(8 + loginBytesSize, userSaltBytesSize);
            QByteArray sessionSalt =
                message.mid(8 + loginBytesSize + userSaltBytesSize, sessionSaltBytesSize);

            handleLoginSalt(login, userSalt, sessionSalt);
        }
            break;
        case NetworkProtocol::UserPlayingForModeMessage:
        {
            if (messageLength < 8) {
                return; /* invalid message */
            }

            int loginBytesSize = (uint)NetworkUtil::getByte(message, 2);
            quint32 userId = NetworkUtil::get4Bytes(message, 4);

            if (messageLength != 8 + loginBytesSize) {
                return; /* invalid message */
            }

            QString login = NetworkUtil::getUtf8String(message, 8, loginBytesSize);

            qDebug() << "received user playing for: id =" << userId
                     << "; login =" << login;

            emit receivedUserPlayingFor(userId, login);
        }
            break;
        case NetworkProtocol::CollectionFetchResponseMessage:
        case NetworkProtocol::CollectionChangeNotificationMessage:
            parseTrackInfoBatchMessage(message, messageType);
            break;
        case NetworkProtocol::ServerNameMessage:
        {
            if (messageLength < 4) {
                return; /* invalid message */
            }

            quint8 nameType = NetworkUtil::getByte(message, 3);
            QString name = NetworkUtil::getUtf8String(message, 4, messageLength - 4);

            qDebug() << "received server name; type:" << nameType << " name:" << name;
            emit receivedServerName(nameType, name);
        }
            break;
        default:
            qDebug() << "received unknown binary message type" << messageType
                     << " with length" << messageLength;
            break; /* unknown message type */
        }
    }

    void ServerConnection::parseTrackInfoBatchMessage(QByteArray const& message,
                                           NetworkProtocol::ServerMessageType messageType)
    {
        qint32 messageLength = message.length();
        if (messageLength < 4) {
            return; /* invalid message */
        }

        bool isNotification =
            messageType == NetworkProtocol::CollectionChangeNotificationMessage;

        int offset = isNotification ? 4 : 8;

        int trackCount = (uint)NetworkUtil::get2Bytes(message, 2);
        if (trackCount == 0
            || messageLength < offset + NetworkProtocol::FILEHASH_BYTECOUNT + 1 + 2 + 2)
        {
            return; /* irrelevant or invalid message */
        }

        AbstractCollectionFetcher* collectionFetcher = nullptr;
        if (!isNotification) {
            quint32 clientReference = NetworkUtil::get4Bytes(message, 4);
            collectionFetcher = _collectionFetchers.value(clientReference, nullptr);
            if (!collectionFetcher) {
                return; /* irrelevant or invalid message */
            }
        }

        QList<int> offsets;
        offsets.append(offset);

        while (true) {
            /* skip hash */
            /* skip availability */
            int titleSize =
                (uint)NetworkUtil::get2Bytes(
                    message, offset + NetworkProtocol::FILEHASH_BYTECOUNT + 1);
            int artistSize =
                (uint)NetworkUtil::get2Bytes(
                    message, offset + NetworkProtocol::FILEHASH_BYTECOUNT + 1 + 2);
            int titleArtistOffset =
                offset + NetworkProtocol::FILEHASH_BYTECOUNT + 1 + 2 + 2;

            if (titleSize > messageLength - titleArtistOffset
                || artistSize > messageLength - titleArtistOffset
                || (titleSize + artistSize) > messageLength - titleArtistOffset)
            {
                return; /* invalid message */
            }

            if (titleArtistOffset + titleSize + artistSize == messageLength) {
                break; /* end of message */
            }

            /* at least one more track info follows */

            /* offset for next track */
            offset = titleArtistOffset + titleSize + artistSize;

            if (offset
                + NetworkProtocol::FILEHASH_BYTECOUNT + 1 + 2 + 2 > messageLength)
            {
                return;  /* invalid message */
            }

            offsets.append(offset);
        }

        qDebug() << "received collection track info message;  track count:" << trackCount;

        if (trackCount != offsets.size()) {
            qDebug() << " invalid message detected: offsets size:" << offsets.size();
            return; /* invalid message */
        }

        /* now read all track info's */
        QList<CollectionTrackInfo> infos;
        infos.reserve(trackCount);
        for (int i = 0; i < trackCount; ++i) {
            offset = offsets[i];

            bool ok;
            FileHash hash = NetworkProtocol::getHash(message, offset, &ok);
            if (!ok || hash.empty()) {
                qDebug() << " invalid message detected: did not read hash correctly;"
                         << "  ok=" << (ok ? "true" : "false");
                return; /* invalid message */
            }
            offset += NetworkProtocol::FILEHASH_BYTECOUNT;

            quint8 availabilityByte = NetworkUtil::getByte(message, offset);
            int titleSize = (uint)NetworkUtil::get2Bytes(message, offset + 1);
            int artistSize = (uint)NetworkUtil::get2Bytes(message, offset + 3);
            offset += 5;

            QString title = NetworkUtil::getUtf8String(message, offset, titleSize);
            QString artist =
                NetworkUtil::getUtf8String(message, offset + titleSize, artistSize);

            CollectionTrackInfo info(hash, availabilityByte & 1, title, artist);
            infos.append(info);
        }

        if (isNotification) {
            emit collectionTracksChanged(infos);
        }
        else {
            collectionFetcher->receivedData(infos);
        }
    }

    void ServerConnection::handleResultMessage(quint16 errorType, quint32 clientReference,
                                               quint32 intData,
                                               QByteArray const& blobData)
    {
        if ((NetworkProtocol::ErrorType)errorType
                == NetworkProtocol::InvalidMessageStructure)
        {
            qDebug() << " errortype = InvalidMessageStructure !!";
        }

        if (clientReference == _userLoginRef) {
            handleUserLoginResult(errorType, intData, blobData);
            return;
        }

        if (clientReference == _userAccountRegistrationRef) {
            handleUserRegistrationResult(errorType, intData, blobData);
            return;
        }

        auto collectionFetcher = _collectionFetchers.value(clientReference, nullptr);
        if (collectionFetcher) {
            if ((NetworkProtocol::ErrorType)errorType == NetworkProtocol::NoError) {
                collectionFetcher->completed();
            }
            else {
                qDebug() << "received ERROR code" << errorType << "from collection fetch";
                collectionFetcher->errorOccurred();
            }

            _collectionFetchers.remove(clientReference);
            return;
        }

        qDebug() << " client reference is unknown:" << clientReference;

        switch ((NetworkProtocol::ErrorType) errorType) {

        default:
            qDebug() << " WARNING: error/result type is unknown/unhandled; "
                     << "intData:" << intData << "";
            break;
        }
    }

    // ============================================================================ //

    AbstractCollectionFetcher::AbstractCollectionFetcher(QObject* parent)
     : QObject(parent)
    {
        //
    }

}
