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

#include "connectedclient.h"

#include "common/filehash.h"
#include "common/networkprotocol.h"
#include "common/networkutil.h"

#include "collectionmonitor.h"
#include "database.h"
#include "generator.h"
//#include "player.h"
#include "queue.h"
#include "queueentry.h"
#include "resolver.h"
#include "server.h"
#include "users.h"

#include <QMap>

namespace PMP {

    ConnectedClient::ConnectedClient(QTcpSocket* socket, Server* server, Player* player,
                                     Generator* generator, Users* users,
                                     CollectionMonitor *collectionMonitor)
     : QObject(server),
       _terminated(false), _socket(socket),
       _server(server), _player(player), _generator(generator), _users(users),
       _binaryMode(false), _clientProtocolNo(-1), _lastSentNowPlayingID(0),
       _userLoggedIn(0)
    {
        auto queue = &player->queue();
        auto resolver = &_player->resolver();

        connect(
            server, &Server::shuttingDown,
            this, &ConnectedClient::terminateConnection
        );

        connect(
            socket, &QTcpSocket::disconnected,
            this, &ConnectedClient::terminateConnection
        );
        connect(socket, &QTcpSocket::readyRead, this, &ConnectedClient::dataArrived);
        connect(
            socket, static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error),
            this, &ConnectedClient::socketError
        );

        connect(player, &Player::volumeChanged, this, &ConnectedClient::volumeChanged);
        connect(
            generator, &Generator::enabledChanged,
            this, &ConnectedClient::dynamicModeStatusChanged
        );
        connect(
            generator, &Generator::noRepetitionSpanChanged,
            this, &ConnectedClient::dynamicModeNoRepetitionSpanChanged
        );
        connect(
            player, &Player::stateChanged,
            this, &ConnectedClient::playerStateChanged
        );
        connect(
            player, &Player::currentTrackChanged,
            this, &ConnectedClient::currentTrackChanged
        );
        connect(
            player, &Player::positionChanged,
            this, &ConnectedClient::trackPositionChanged
        );
        connect(
            player, &Player::userPlayingForChanged,
            this, &ConnectedClient::onUserPlayingForChanged
        );

        connect(
            queue, &Queue::entryRemoved,
            this, &ConnectedClient::queueEntryRemoved
        );
        connect(
            queue, &Queue::entryAdded,
            this, &ConnectedClient::queueEntryAdded
        );
        connect(
            queue, &Queue::entryMoved,
            this, &ConnectedClient::queueEntryMoved
        );

        connect(
            resolver, &Resolver::fullIndexationRunStatusChanged,
            this, &ConnectedClient::onFullIndexationRunStatusChanged
        );

        /* send greeting */
        sendTextCommand("PMP 0.1 Welcome!");
    }

    ConnectedClient::~ConnectedClient() {
        qDebug() << "ConnectedClient: destructor called";
        _socket->deleteLater();
    }

    void ConnectedClient::terminateConnection() {
        qDebug() << "terminateConnection() called";
        if (_terminated) return;
        qDebug() << " will terminate and cleanup connection now";
        _terminated = true;
        _socket->close();
        _textReadBuffer.clear();
        this->deleteLater();
    }

    void ConnectedClient::dataArrived() {
        if (_terminated) {
            qDebug() << "ConnectedClient::dataArrived called on a terminated connection???";
            return;
        }

        if (_binaryMode && _clientProtocolNo >= 0) {
            readBinaryCommands();
            return;
        }

        if (!_binaryMode) {
            /* textual command mode */
            readTextCommands();
        }

        /* not changed to binary mode? */
        if (!_binaryMode) { return; }

        /* do we still need to read the binary header? */
        if (_clientProtocolNo < 0) {
            if (_socket->bytesAvailable() < 5) {
                /* not enough data */
                return;
            }

            char heading[5];
            _socket->read(heading, 5);

            if (heading[0] != 'P'
                || heading[1] != 'M'
                || heading[2] != 'P')
            {
                terminateConnection();
                return;
            }

            _clientProtocolNo = (heading[3] << 8) + heading[4];
            qDebug() << "client supports protocol " << QString::number(_clientProtocolNo);
        }

        readBinaryCommands();
    }

    void ConnectedClient::socketError(QAbstractSocket::SocketError error) {
        switch (error) {
            case QAbstractSocket::RemoteHostClosedError:
                this->terminateConnection();
                break;
            default:
                // problem ??

                break;
        }
    }

    void ConnectedClient::readTextCommands() {
        while (!_terminated && !_binaryMode) {
            bool hadSemicolon = false;
            char c;
            while (_socket->getChar(&c)) {
                if (c == ';') {
                    hadSemicolon = true;
                    /* skip an optional newline after the semicolon */
                    if (_socket->peek(&c, 1) > 0 && c == '\r') _socket->read(&c, 1);
                    if (_socket->peek(&c, 1) > 0 && c == '\n') _socket->read(&c, 1);
                    break;
                }

                _textReadBuffer.append(c); /* not for semicolons */
            }

            if (!hadSemicolon) {
                break; /* no complete text command in received data */
            }

            QString commandString = QString::fromUtf8(_textReadBuffer);

            _textReadBuffer.clear(); /* text was consumed completely */

            executeTextCommand(commandString);
        }
    }

    void ConnectedClient::executeTextCommand(QString const& commandText) {
        qDebug() << "received text commandline:" << commandText;

        int spaceIndex = commandText.indexOf(' ');
        QString command;

        if (spaceIndex < 0) { /* command without arguments */
            command = commandText;

            if (command == "play") {
                _player->play();
            }
            else if (command == "pause") {
                _player->pause();
            }
            else if (command == "skip") {
                _player->skip();
            }
            else if (command == "volume") {
                /* 'volume' without arguments sends current volume */
                sendVolumeMessage();
            }
            else if (command == "state") {
                /* pretend state has changed, in order to send state info */
                playerStateChanged(_player->state());
            }
            else if (command == "nowplaying") {
                /* pretend current track has changed, in order to send current track info */
                currentTrackChanged(_player->nowPlaying());
            }
            else if (command == "queue") {
                sendTextualQueueInfo();
            }
            else if (command == "shutdown") {
                _server->shutdown();
            }
            else if (command == "binary") {
                /* switch to binary mode */
                _binaryMode = true;
                /* tell the client that all further communication will be in binary mode */
                sendTextCommand("binary");

                char binaryHeader[5];
                binaryHeader[0] = 'P';
                binaryHeader[1] = 'M';
                binaryHeader[2] = 'P';
                binaryHeader[3] = 0; /* protocol number; high byte */
                binaryHeader[4] = 1; /* protocol number; low byte */
                _socket->write(binaryHeader, sizeof(binaryHeader));
            }
            else {
                /* unknown command ???? */
                qDebug() << " unknown text command: " << command;
            }

            return;
        }

        /* split command at the space; don't include the space in the parts */
        command = commandText.left(spaceIndex);
        QString rest = commandText.mid(spaceIndex + 1);
        spaceIndex = rest.indexOf(' ');
        QString arg1;

        if (spaceIndex < 0) { /* one argument only */
            arg1 = rest;

            /* 'volume' with one argument changes current volume */
            if (command == "volume") {
                bool ok;
                uint volume = arg1.toUInt(&ok);
                if (ok && volume >= 0 && volume <= 100) {
                    _player->setVolume(volume);
                }
            }
            else {
                /* unknown command ???? */
                qDebug() << " unknown text command: " << command;
            }

            return;
        }

        /* two arguments or more */

        arg1 = rest.left(spaceIndex);
        rest = rest.mid(spaceIndex + 1);
        spaceIndex = rest.indexOf(' ');
        QString arg2;

        if (spaceIndex < 0) { /* two arguments only */
            arg2 = rest;

            if (command == "qmove") {
                bool ok;
                uint queueID = arg1.toUInt(&ok);
                if (!ok || queueID == 0) return;

                if (!arg2.startsWith("+") && !arg2.startsWith("-")) return;
                int moveDiff = arg2.toInt(&ok);
                if (!ok || moveDiff == 0) return;

                _player->queue().move(queueID, moveDiff);
            }
            else {
                /* unknown command ???? */
                qDebug() << " unknown text command: " << command;
            }

            return;
        }

        /* unknown command ???? */
        qDebug() << " unknown text command: " << command;
    }

    void ConnectedClient::sendTextCommand(QString const& command) {
        if (_terminated) {
            qDebug() << "sendTextCommand: cannot proceed because connection is terminated";
            return;
        }

        _socket->write((command + ";").toUtf8());
    }

    void ConnectedClient::sendBinaryMessage(QByteArray const& message) {
        if (_terminated) {
            qDebug() << "sendBinaryMessage: cannot proceed because connection is terminated";
            return;
        }

        if (!_binaryMode) {
            qDebug() << "sendBinaryMessage: cannot send this when not in binary mode";
            return; /* only supported in binary mode */
        }

        quint32 length = message.length();
        qDebug() << "   need to send a binary message of length" << length;

        char lengthArr[4];
        lengthArr[0] = (length >> 24) & 255;
        lengthArr[1] = (length >> 16) & 255;
        lengthArr[2] = (length >> 8) & 255;
        lengthArr[3] = length & 255;

        //qDebug() << "      arr[3]=" << ((int)(quint8)lengthArr[3]);

        _socket->write(lengthArr, sizeof(lengthArr));
        _socket->write(message.data(), length);
    }

    void ConnectedClient::sendStateInfo() {
        qDebug() << "sending state info";

        Player::State state = _player->state();
        quint8 stateNum = 0;
        switch (state) {
        case Player::Stopped:
            stateNum = 1;
            break;
        case Player::Playing:
            stateNum = 2;
            break;
        case Player::Paused:
            stateNum = 3;
            break;
        }

        quint64 position = _player->playPosition();
        quint8 volume = _player->volume();

        quint32 queueLength = _player->queue().length();

        QueueEntry const* nowPlaying = _player->nowPlaying();
        quint32 queueID = nowPlaying ? nowPlaying->queueID() : 0;

        QByteArray message;
        message.reserve(20);
        NetworkUtil::append2Bytes(message, NetworkProtocol::PlayerStateMessage);
        NetworkUtil::appendByte(message, stateNum);
        NetworkUtil::appendByte(message, volume);
        NetworkUtil::append4Bytes(message, queueLength);
        NetworkUtil::append4Bytes(message, queueID);
        NetworkUtil::append8Bytes(message, position);

        sendBinaryMessage(message);

        _lastSentNowPlayingID = queueID;
    }

    void ConnectedClient::sendVolumeMessage() {
        quint8 volume = _player->volume();

        if (!_binaryMode) {
            sendTextCommand("volume " + QString::number(volume));
            return;
        }

        QByteArray message;
        message.reserve(3);
        NetworkUtil::append2Bytes(message, NetworkProtocol::VolumeChangedMessage);
        NetworkUtil::appendByte(message, volume);

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendDynamicModeStatusMessage() {
        quint8 enabled = _generator->enabled() ? 1 : 0;
        quint32 noRepetitionSpan = (quint32)_generator->noRepetitionSpan();

        QByteArray message;
        message.reserve(7);
        NetworkUtil::append2Bytes(message, NetworkProtocol::DynamicModeStatusMessage);
        NetworkUtil::appendByte(message, enabled);
        NetworkUtil::append4Bytes(message, noRepetitionSpan);

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendUserPlayingForModeMessage() {
        quint32 user = _player->userPlayingFor();
        QString login = _users->getUserLogin(user);
        QByteArray loginBytes = login.toUtf8();

        QByteArray message;
        message.reserve(4 + 4 + loginBytes.length());
        NetworkUtil::append2Bytes(message, NetworkProtocol::UserPlayingForModeMessage);
        NetworkUtil::appendByte(message, (quint8)loginBytes.size());
        NetworkUtil::appendByte(message, 0); /* unused */
        NetworkUtil::append4Bytes(message, user);
        message += loginBytes;

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendEventNotificationMessage(quint8 event) {
        qDebug() << "   sending server event number" << event;

        QByteArray message;
        message.reserve(2 + 2);
        NetworkUtil::append2Bytes(message, NetworkProtocol::ServerEventNotificationMessage);
        NetworkUtil::appendByte(message, event);
        NetworkUtil::appendByte(message, 0); /* unused */

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendServerInstanceIdentifier() {
        QUuid uuid = _server->uuid();

        QByteArray message;
        message.reserve(2 + 16);
        NetworkUtil::append2Bytes(message, NetworkProtocol::ServerInstanceIdentifierMessage);
        message.append(uuid.toRfc4122());

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendUsersList() {
        QList<UserIdAndLogin> users = _users->getUsers();

        QByteArray message;
        message.reserve(2 + 2 + users.size() * (4 + 1 + 20)); /* only an approximation */
        NetworkUtil::append2Bytes(message, NetworkProtocol::UsersListMessage);
        NetworkUtil::append2Bytes(message, (quint16)users.size());

        Q_FOREACH(UserIdAndLogin user, users) {
            NetworkUtil::append4Bytes(message, user.first); /* ID */

            QByteArray loginNameBytes = user.second.toUtf8();
            loginNameBytes.truncate(255);

            NetworkUtil::appendByte(message, (quint8)loginNameBytes.size());
            message += loginNameBytes;
        }

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendQueueContentMessage(quint32 startOffset, quint8 length) {
        Queue& queue = _player->queue();
        uint queueLength = queue.length();

        if (startOffset >= queueLength) {
            length = 0;
        }
        else if (startOffset > queueLength - length) {
            length = queueLength - startOffset;
        }

        if (length > 255) { length = 255; }

        QList<QueueEntry*> entries = queue.entries(startOffset, (length == 0) ? -1 : length);

        QByteArray message;
        message.reserve(10 + entries.length() * 4);
        NetworkUtil::append2Bytes(message, NetworkProtocol::QueueContentsMessage);
        NetworkUtil::append4Bytes(message, queueLength);
        NetworkUtil::append4Bytes(message, startOffset);

        QueueEntry* entry;
        foreach(entry, entries) {
            NetworkUtil::append4Bytes(message, entry->queueID());
        }

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendQueueEntryRemovedMessage(quint32 offset, quint32 queueID) {
        QByteArray message;
        message.reserve(10);
        NetworkUtil::append2Bytes(message, NetworkProtocol::QueueEntryRemovedMessage);
        NetworkUtil::append4Bytes(message, offset);
        NetworkUtil::append4Bytes(message, queueID);

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendQueueEntryAddedMessage(quint32 offset, quint32 queueID) {
        QByteArray message;
        message.reserve(10);
        NetworkUtil::append2Bytes(message, NetworkProtocol::QueueEntryAddedMessage);
        NetworkUtil::append4Bytes(message, offset);
        NetworkUtil::append4Bytes(message, queueID);

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendQueueEntryMovedMessage(quint32 fromOffset, quint32 toOffset,
                                                     quint32 queueID)
    {
        QByteArray message;
        message.reserve(14);
        NetworkUtil::append2Bytes(message, NetworkProtocol::QueueEntryMovedMessage);
        NetworkUtil::append4Bytes(message, fromOffset);
        NetworkUtil::append4Bytes(message, toOffset);
        NetworkUtil::append4Bytes(message, queueID);

        sendBinaryMessage(message);
    }

    quint16 ConnectedClient::createTrackStatusFor(QueueEntry* entry) {
        if (entry->isTrack()) {
            return NetworkProtocol::createTrackStatusForTrack();
        }
        else {
            /* TODO: there will be more pseudos; differentiate between them... */
            return NetworkProtocol::createTrackStatusForBreakPoint();
        }
    }

    void ConnectedClient::sendTrackInfoMessage(quint32 queueID) {
        QueueEntry* track = _player->queue().lookup(queueID);
        if (track == 0) { return; /* sorry, cannot send */ }

        track->checkTrackData(_player->resolver());

        auto trackStatus = createTrackStatusFor(track);
        QString title = track->title();
        QString artist = track->artist();
        qint32 length = track->lengthInSeconds(); /* SIGNED NUMBER!! */

        const int maxSize = (1 << 16) - 1;

        /* worst case: 4 bytes in UTF-8 for each char */
        title.truncate(maxSize / 4);
        artist.truncate(maxSize / 4);

        QByteArray titleData = title.toUtf8();
        QByteArray artistData = artist.toUtf8();

        QByteArray message;
        message.reserve((2 + 1 + 1 + 4 * 3) + titleData.size() + artistData.size());
        NetworkUtil::append2Bytes(message, NetworkProtocol::TrackInfoMessage);
        NetworkUtil::append2Bytes(message, trackStatus);
        NetworkUtil::append4Bytes(message, queueID);
        NetworkUtil::append4Bytes(message, length); /* length in seconds, SIGNED */
        NetworkUtil::append2Bytes(message, titleData.size());
        NetworkUtil::append2Bytes(message, artistData.size());
        message += titleData;
        message += artistData;

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendTrackInfoMessage(QList<quint32> const& queueIDs) {
        if (queueIDs.empty()) {
            return;
        }

        const int maxSize = (1 << 16) - 1;

        /* not too big? */
        if (queueIDs.size() > maxSize) {
            /* TODO: maybe delay the second part? */
            sendTrackInfoMessage(queueIDs.mid(0, maxSize));
            sendTrackInfoMessage(queueIDs.mid(maxSize));
            return;
        }

        QByteArray message;
        /* reserve some memory, take a guess at how much space we will probably need */
        message.reserve(
            4 + queueIDs.size() * (2 + 12 + /*title*/20 + /*artist*/15)
        );

        NetworkUtil::append2Bytes(message, NetworkProtocol::BulkTrackInfoMessage);
        NetworkUtil::append2Bytes(message, (uint)queueIDs.size());

        Queue& queue = _player->queue();

        /* TODO: bug: concurrency issue here when a QueueEntry has just been deleted */

        Q_FOREACH(quint32 queueID, queueIDs) {
            QueueEntry* track = queue.lookup(queueID);
            auto trackStatus =
                track ? createTrackStatusFor(track)
                      : NetworkProtocol::createTrackStatusUnknownId();
            NetworkUtil::append2Bytes(message, trackStatus);
        }
        if (queueIDs.size() % 2 != 0) {
            NetworkUtil::append2Bytes(message, 0); /* filler */
        }

        Q_FOREACH(quint32 queueID, queueIDs) {
            QueueEntry* track = queue.lookup(queueID);
            if (track == 0) { continue; /* ID not found */ }

            track->checkTrackData(_player->resolver());

            QString title = track->title();
            QString artist = track->artist();
            qint32 length = track->lengthInSeconds(); /* SIGNED NUMBER!! */

            /* worst case: 4 bytes in UTF-8 for each char */
            title.truncate(maxSize / 4);
            artist.truncate(maxSize / 4);

            QByteArray titleData = title.toUtf8();
            QByteArray artistData = artist.toUtf8();

            NetworkUtil::append4Bytes(message, queueID);
            NetworkUtil::append4Bytes(message, length); /* length in seconds, SIGNED */
            NetworkUtil::append2Bytes(message, (uint)titleData.size());
            NetworkUtil::append2Bytes(message, (uint)artistData.size());
            message += titleData;
            message += artistData;
        }

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendPossibleTrackFilenames(quint32 queueID,
                                                     QList<QString> const& names)
    {
        QByteArray message;
        //message.reserve(2 + );
        NetworkUtil::append2Bytes(message, NetworkProtocol::PossibleFilenamesForQueueEntryMessage);

        NetworkUtil::append4Bytes(message, queueID);

        Q_FOREACH(QString name, names) {
            QByteArray nameBytes = name.toUtf8();
            NetworkUtil::append4Bytes(message, nameBytes.size());
            message += nameBytes;
        }

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendNewUserAccountSaltMessage(QString login,
                                                        QByteArray const& salt)
    {
        QByteArray loginBytes = login.toUtf8();

        QByteArray message;
        message.reserve(4 + loginBytes.size() + salt.size());
        NetworkUtil::append2Bytes(message, NetworkProtocol::NewUserAccountSaltMessage);
        NetworkUtil::appendByte(message, (quint8)loginBytes.size());
        NetworkUtil::appendByte(message, (quint8)salt.size());

        message += loginBytes;
        message += salt;

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendUserLoginSaltMessage(QString login,
                                                   const QByteArray &userSalt,
                                                   const QByteArray &sessionSalt)
    {
        QByteArray loginBytes = login.toUtf8();

        QByteArray message;
        message.reserve(4 + 4 + loginBytes.size() + userSalt.size() + sessionSalt.size());
        NetworkUtil::append2Bytes(message, NetworkProtocol::UserLoginSaltMessage);
        NetworkUtil::append2Bytes(message, 0); /* unused */
        NetworkUtil::appendByte(message, (quint8)loginBytes.size());
        NetworkUtil::appendByte(message, (quint8)userSalt.size());
        NetworkUtil::appendByte(message, (quint8)sessionSalt.size());
        NetworkUtil::appendByte(message, 0); /* unused */

        message += loginBytes;
        message += userSalt;
        message += sessionSalt;

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendSuccessMessage(quint32 clientReference, quint32 intData)
    {
        sendResultMessage(NetworkProtocol::NoError, clientReference, intData);
    }

    void ConnectedClient::sendSuccessMessage(quint32 clientReference, quint32 intData,
                                             QByteArray const& blobData)
    {
        sendResultMessage(NetworkProtocol::NoError, clientReference, intData, blobData);
    }

    void ConnectedClient::sendResultMessage(NetworkProtocol::ErrorType errorType,
                                            quint32 clientReference, quint32 intData)
    {
        QByteArray data; /* empty data */
        sendResultMessage(errorType, clientReference, intData, data);
    }

    void ConnectedClient::sendResultMessage(NetworkProtocol::ErrorType errorType,
                                            quint32 clientReference, quint32 intData,
                                            QByteArray const& blobData)
    {
        QByteArray message;
        message.reserve(2 + 2 + 4 + 4 + blobData.size());
        NetworkUtil::append2Bytes(message, NetworkProtocol::SimpleResultMessage);
        NetworkUtil::append2Bytes(message, errorType);
        NetworkUtil::append4Bytes(message, clientReference);
        NetworkUtil::append4Bytes(message, intData);

        message += blobData;

        sendBinaryMessage(message);
    }

    void ConnectedClient::volumeChanged(int volume) {
        sendVolumeMessage();
    }

    void ConnectedClient::dynamicModeStatusChanged(bool enabled) {
        sendDynamicModeStatusMessage();
    }

    void ConnectedClient::dynamicModeNoRepetitionSpanChanged(int seconds) {
        sendDynamicModeStatusMessage();
    }

    void ConnectedClient::onUserPlayingForChanged(quint32 user) {
        sendUserPlayingForModeMessage();
    }

    void ConnectedClient::onFullIndexationRunStatusChanged(bool running) {
        sendEventNotificationMessage(running ? 1 : 2);
    }

    void ConnectedClient::playerStateChanged(Player::State state) {
        if (_binaryMode) {
            sendStateInfo();
            return;
        }

        switch (state) {
        case Player::Playing:
            sendTextCommand("playing");
            break;
        case Player::Paused:
            sendTextCommand("paused");
            break;
        case Player::Stopped:
            sendTextCommand("stopped");
            break;
        }
    }

    void ConnectedClient::currentTrackChanged(QueueEntry const* entry) {
        if (_binaryMode) {
            sendStateInfo();
            return;
        }

        if (entry == 0) {
            sendTextCommand("nowplaying nothing");
            return;
        }

        int seconds = entry->lengthInSeconds();
        FileHash const* hash = entry->hash();

        sendTextCommand(
            "nowplaying track\n QID: " + QString::number(entry->queueID())
             + "\n position: " + QString::number(_player->playPosition())
             + "\n title: " + entry->title()
             + "\n artist: " + entry->artist()
             + "\n length: " + (seconds < 0 ? "?" : QString::number(seconds))
             + " sec\n hash length: " + (hash == 0 ? "?" : QString::number(hash->length()))
             + "\n hash SHA-1: " + (hash == 0 ? "?" : hash->SHA1().toHex())
             + "\n hash MD5: " + (hash == 0 ? "?" : hash->MD5().toHex())
        );
    }

    void ConnectedClient::trackPositionChanged(qint64 position) {
        if (_binaryMode) {
            sendStateInfo();
            return;
        }

        //sendTextCommand("position " + QString::number(position));
    }

    void ConnectedClient::sendTextualQueueInfo() {
        Queue& queue = _player->queue();
        QList<QueueEntry*> queueContent = queue.entries(0, 10);

        QString command =
            "queue length " + QString::number(queue.length())
            + "\nIndex|  QID  | Length | Title                          | Artist";

        command.reserve(command.size() + 100 * queueContent.size());

        Resolver& resolver = _player->resolver();
        QueueEntry* entry;
        uint index = 0;
        foreach(entry, queueContent) {
            ++index;
            entry->checkTrackData(resolver);

            int lengthInSeconds = entry->lengthInSeconds();

            command += "\n";
            command += QString::number(index).rightJustified(5);
            command += "|";
            command += QString::number(entry->queueID()).rightJustified(7);
            command += "|";

            if (lengthInSeconds < 0) {
                command += "        |";
            }
            else {
                int sec = lengthInSeconds % 60;
                int min = (lengthInSeconds / 60) % 60;
                int hrs = lengthInSeconds / 3600;

                command += QString::number(hrs).rightJustified(2, '0');
                command += ":";
                command += QString::number(min).rightJustified(2, '0');
                command += ":";
                command += QString::number(sec).rightJustified(2, '0');
                command += "|";
            }

            command += entry->title().leftJustified(32);
            command += "|";
            command += entry->artist();
        }

        sendTextCommand(command);
    }

    void ConnectedClient::queueEntryRemoved(quint32 offset, quint32 queueID) {
        sendQueueEntryRemovedMessage(offset, queueID);
    }

    void ConnectedClient::queueEntryAdded(quint32 offset, quint32 queueID) {
        sendQueueEntryAddedMessage(offset, queueID);
    }

    void ConnectedClient::queueEntryMoved(quint32 fromOffset, quint32 toOffset,
                                          quint32 queueID)
    {
        sendQueueEntryMovedMessage(fromOffset, toOffset, queueID);
    }

    void ConnectedClient::readBinaryCommands() {
        char lengthBytes[4];

        while
            (_socket->isOpen()
            && _socket->peek(lengthBytes, sizeof(lengthBytes)) == sizeof(lengthBytes))
        {
            quint32 messageLength = NetworkUtil::get4Bytes(lengthBytes);

            if (_socket->bytesAvailable() - sizeof(lengthBytes) < messageLength) {
                qDebug() << "waiting for incoming message with length" << messageLength
                         << " --- only partially received";
                break; /* message not complete yet */
            }

            //qDebug() << "received complete binary message with length" << messageLength;

            _socket->read(lengthBytes, sizeof(lengthBytes)); /* consume the length */
            QByteArray message = _socket->read(messageLength);

            handleBinaryMessage(message);
        }
    }

    void ConnectedClient::handleBinaryMessage(QByteArray const& message) {
        qint32 messageLength = message.length();

        if (messageLength < 2) {
            qDebug() << "received invalid binary message (less than 2 bytes)";
            return; /* invalid message */
        }

        int messageType = NetworkUtil::get2Bytes(message, 0);

        switch ((NetworkProtocol::ClientMessageType)messageType) {
        case NetworkProtocol::SingleByteActionMessage:
        {
            if (messageLength != 3) {
                return; /* invalid message */
            }

            quint8 actionType = NetworkUtil::getByte(message, 2);

            if (actionType >= 100 && actionType <= 200) {
                qDebug() << "received CHANGE VOLUME command, volume"
                         << ((uint)actionType - 100);
                _player->setVolume(actionType - 100);
                break;
            }

            switch(actionType) {
            case 1:
                qDebug() << "received PLAY command";
                _player->play();
                break;
            case 2:
                qDebug() << "received PAUSE command";
                _player->pause();
                break;
            case 3:
                qDebug() << "received SKIP command";
                _player->skip();
                break;
            case 4:
                qDebug() << "received INSERT BREAK AT FRONT command";
                _player->queue().insertBreakAtFront();
                break;
            case 10: /* request for state info */
                qDebug() << "received request for player status";
                sendStateInfo();
                break;
            case 11: /* request for status of dynamic mode */
                qDebug() << "received request for dynamic mode status";
                sendDynamicModeStatusMessage();
                break;
            case 12:
                qDebug() << "received request for server instance UUID";
                sendServerInstanceIdentifier();
                break;
            case 13:
                qDebug() << "received request for list of user accounts";
                sendUsersList();
                break;
            case 14:
                qDebug() << "received request for PUBLIC/PERSONAL mode status";
                sendUserPlayingForModeMessage();
                break;
            case 15:
                qDebug() << "received request for (full) indexation running status";
                onFullIndexationRunStatusChanged(
                    _player->resolver().fullIndexationRunning()
                );
                break;
            case 20: /* enable dynamic mode */
                qDebug() << "received ENABLE DYNAMIC MODE command";
                _generator->enable();
                break;
            case 21: /* disable dynamic mode */
                qDebug() << "received DISABLE DYNAMIC MODE command";
                _generator->disable();
                break;
            case 22: /* request queue expansion */
                qDebug() << "received QUEUE EXPANSION command";
                _generator->requestQueueExpansion();
                break;
            case 23:
                qDebug() << "received TRIM QUEUE command";
                _player->queue().trim(10); /* TODO: get the '10' from elsewhere */
                break;
            case 30: /* switch to public mode */
                qDebug() << "received SWITCH TO PUBLIC MODE command";
                _player->setUserPlayingFor(0);
                break;
            case 31: /* switch to personal mode */
                qDebug() << "received SWITCH TO PERSONAL MODE command";
                if (_userLoggedIn > 0) {
                    qDebug() << " switching to personal mode for user "
                             << _userLoggedInName;
                    _player->setUserPlayingFor(_userLoggedIn);
                }
                break;
            case 40:
                qDebug() << "received START FULL INDEXATION command";
                _player->resolver().startFullIndexation();
                break;
            case 99:
                qDebug() << "received SHUTDOWN command";
                _server->shutdown();
                break;
            default:
                qDebug() << "received unrecognized single-byte action type:"
                         << (int)actionType;
                break; /* unknown action type */
            }
        }
            break;
        case NetworkProtocol::TrackInfoRequestMessage:
        {
            if (messageLength != 6) {
                return; /* invalid message */
            }

            quint32 queueID = NetworkUtil::get4Bytes(message, 2);

            if (queueID <= 0) {
                return; /* invalid queue ID */
            }

            qDebug() << "received track info request for Q-ID" << queueID;

            sendTrackInfoMessage(queueID);
        }
            break;
        case NetworkProtocol::BulkTrackInfoRequestMessage:
        {
            if (messageLength < 6 || (messageLength - 2) % 4 != 0) {
                return; /* invalid message */
            }

            QList<quint32> QIDs;
            QIDs.reserve((messageLength - 2) / 4);

            qint32 offset = 2;
            while (offset <= messageLength - 4) {
                quint32 queueID = NetworkUtil::get4Bytes(message, offset);

                if (queueID > 0) {
                    QIDs.append(queueID);
                }

                offset += 4;
            }

            qDebug() << "received bulk track info request for" << QIDs.size() << "tracks";

            sendTrackInfoMessage(QIDs);
        }
            break;
        case NetworkProtocol::QueueFetchRequestMessage:
        {
            if (messageLength != 7) {
                return; /* invalid message */
            }

            quint32 startOffset = NetworkUtil::get4Bytes(message, 2);
            quint8 length = NetworkUtil::getByte(message, 6);

            qDebug() << "received queue fetch request; offset:" << startOffset << "  length:" << length;

            sendQueueContentMessage(startOffset, length);
        }
            break;
        case NetworkProtocol::QueueEntryRemovalRequestMessage:
        {
            if (messageLength != 6) {
                return; /* invalid message */
            }

            quint32 queueID = NetworkUtil::get4Bytes(message, 2);
            qDebug() << "received removal request for QID" << queueID;

            if (queueID <= 0) {
                return; /* invalid queue ID */
            }

            _player->queue().remove(queueID);
        }
            break;
        case NetworkProtocol::GeneratorNonRepetitionChangeMessage:
        {
            if (messageLength != 6) {
                return; /* invalid message */
            }

            qint32 intervalMinutes = (qint32)NetworkUtil::get4Bytes(message, 2);
            qDebug() << "received change request for generator non-repetition interval;  minutes:" << intervalMinutes;

            if (intervalMinutes < 0) {
                return; /* invalid message */
            }

            _generator->setNoRepetitionSpan(intervalMinutes);
        }
            break;
        case NetworkProtocol::PossibleFilenamesForQueueEntryRequestMessage:
        {
            if (messageLength != 6) {
                return; /* invalid message */
            }

            quint32 queueID = NetworkUtil::get4Bytes(message, 2);
            qDebug() << "received request for possible filenames of QID" << queueID;

            if (queueID <= 0) {
                return; /* invalid queue ID */
            }

            QueueEntry* entry = _player->queue().lookup(queueID);
            if (entry == 0) {
                return; /* not found :-/ */
            }

            const FileHash* hash = entry->hash();
            if (hash == 0) {
                /* hash not found */
                /* TODO: register callback to resume this once the hash becomes known */
                return;
            }

            uint hashID = _player->resolver().getID(*hash);

            /* FIXME: do this in another thread, it will slow down the main thread */

            auto db = Database::getDatabaseForCurrentThread();
            if (!db) {
                return; /* database unusable */
            }

            QList<QString> filenames = db->getFilenames(hashID);

            sendPossibleTrackFilenames(queueID, filenames);
        }
            break;
        case NetworkProtocol::PlayerSeekRequestMessage:
        {
            if (messageLength != 14) {
                return; /* invalid message */
            }

            quint32 queueID = NetworkUtil::get4Bytes(message, 2);
            qint64 position = (qint64)NetworkUtil::get8Bytes(message, 6);

            qDebug() << "received seek command; QID:" << queueID
                     << "  position:" << position;

            if (queueID != _player->nowPlayingQID()) {
                return; /* invalid queue ID */
            }

            if (position < 0) return; /* invalid position */

            _player->seekTo(position);
        }
            break;
        case NetworkProtocol::QueueEntryMoveRequestMessage:
        {
            if (messageLength != 8) {
                return; /* invalid message */
            }

            qint16 move = NetworkUtil::get2Bytes(message, 2);
            quint32 queueID = NetworkUtil::get4Bytes(message, 4);

            qDebug() << "received track move command; QID:" << queueID
                     << " move:" << move;

            _player->queue().move(queueID, move);
        }
            break;
        case NetworkProtocol::InitiateNewUserAccountMessage:
        {
            if (messageLength <= 8) {
                return; /* invalid message */
            }

            qint32 loginLength = (quint32)NetworkUtil::getByte(message, 2);
            quint32 clientReference = NetworkUtil::get4Bytes(message, 4);

            if (messageLength - 8 != loginLength) {
                return; /* invalid message */
            }

            qDebug() << "received initiate-new-user-account request; clientRef:"
                     << clientReference;

            QByteArray loginBytes = message.mid(8);
            QString login = QString::fromUtf8(loginBytes);

            QByteArray salt;
            Users::ErrorCode result = Users::generateSaltForNewAccount(login, salt);

            if (result == Users::Successfull) {
                _userAccountRegistering = login;
                _saltForUserAccountRegistering = salt;
                sendNewUserAccountSaltMessage(login, salt);
            }
            else {
                sendResultMessage(
                    Users::toNetworkProtocolError(result), clientReference, 0, loginBytes
                );
            }
        }
            break;
        case NetworkProtocol::FinishNewUserAccountMessage:
        {
            if (messageLength <= 8) {
                return; /* invalid message */
            }

            qint32 loginLength = (quint32)NetworkUtil::getByte(message, 2);
            qint32 saltLength = (quint32)NetworkUtil::getByte(message, 3);
            quint32 clientReference = NetworkUtil::get4Bytes(message, 4);

            qDebug() << "received finish-new-user-account request; clientRef:"
                     << clientReference;

            if (messageLength - 8 <= loginLength + saltLength) {
                sendResultMessage(
                    NetworkProtocol::InvalidMessageStructure, clientReference, 0
                );
                return; /* invalid message */
            }

            QByteArray loginBytes = message.mid(8, loginLength);
            QString login = QString::fromUtf8(loginBytes);
            QByteArray saltFromClient = message.mid(8 + loginLength, saltLength);

            if (login != _userAccountRegistering
                || saltFromClient != _saltForUserAccountRegistering)
            {
                sendResultMessage(
                    NetworkProtocol::UserAccountRegistrationMismatch,
                    clientReference, 0, loginBytes
                );
                return;
            }

            /* clean up state */
            _userAccountRegistering = "";
            _saltForUserAccountRegistering.clear();

            QByteArray hashedPasswordFromClient =
                message.mid(8 + loginLength + saltLength);

            /* simple way to get the expected length of the hashed password */
            QByteArray hashTest =
                NetworkProtocol::hashPassword(saltFromClient, "test");

            if (hashedPasswordFromClient.size() != hashTest.size()) {
                sendResultMessage(
                    NetworkProtocol::InvalidMessageStructure, clientReference, 0
                );
                return;
            }

            QPair<Users::ErrorCode, quint32> result =
                _users->registerNewAccount(
                    login, saltFromClient, hashedPasswordFromClient
                );

            /* send error or success message */
            sendResultMessage(
                Users::toNetworkProtocolError(result.first),
                clientReference, result.second
            );
        }
            break;
        case NetworkProtocol::InitiateLoginMessage:
        {
            if (messageLength <= 8) {
                return; /* invalid message */
            }

            qint32 loginLength = (quint32)NetworkUtil::getByte(message, 2);
            quint32 clientReference = NetworkUtil::get4Bytes(message, 4);

            if (messageLength - 8 != loginLength) {
                return; /* invalid message */
            }

            qDebug() << "received initiate-login request; clientRef:"
                     << clientReference;

            QByteArray loginBytes = message.mid(8);
            QString login = QString::fromUtf8(loginBytes);

            User user;
            bool userLookup = _users->getUserByLogin(login, user);

            if (!userLookup) { /* user does not exist */
                sendResultMessage(
                    NetworkProtocol::InvalidUserAccountName, clientReference, 0,
                    loginBytes
                );
                return;
            }

            QByteArray userSalt = user.salt;
            QByteArray sessionSalt = Users::generateSalt();

            _userAccountLoggingIn = user.login;
            _sessionSaltForUserLoggingIn = sessionSalt;

            sendUserLoginSaltMessage(login, userSalt, sessionSalt);
        }
            break;
        case NetworkProtocol::FinishLoginMessage:
        {
            if (messageLength <= 12) {
                return; /* invalid message */
            }

            qint32 loginLength = (quint32)NetworkUtil::getByte(message, 4);
            qint32 userSaltLength = (quint32)NetworkUtil::getByte(message, 5);
            qint32 sessionSaltLength = (quint32)NetworkUtil::getByte(message, 6);
            qint32 hashedPasswordLength = (quint32)NetworkUtil::getByte(message, 7);
            quint32 clientReference = NetworkUtil::get4Bytes(message, 8);

            qDebug() << "received finish-login request; clientRef:"
                     << clientReference;

            if (messageLength - 12 !=
                    loginLength + userSaltLength + sessionSaltLength
                    + hashedPasswordLength)
            {
                sendResultMessage(
                    NetworkProtocol::InvalidMessageStructure, clientReference, 0
                );
                return; /* invalid message */
            }

            QByteArray loginBytes = message.mid(12, loginLength);
            QString login = QString::fromUtf8(loginBytes);

            User user;
            bool userLookup = _users->getUserByLogin(login, user);

            if (!userLookup) { /* user does not exist */
                sendResultMessage(
                    NetworkProtocol::InvalidUserAccountName, clientReference, 0,
                    loginBytes
                );
                return;
            }

            qDebug() << " user lookup successfull: user id =" << user.id
                     << "; login =" << user.login;

            QByteArray userSaltFromClient = message.mid(12 + loginLength, userSaltLength);
            QByteArray sessionSaltFromClient =
                message.mid(12 + loginLength + userSaltLength, sessionSaltLength);

            if (login != _userAccountLoggingIn
                || userSaltFromClient != user.salt
                || sessionSaltFromClient != _sessionSaltForUserLoggingIn)
            {
                sendResultMessage(
                    NetworkProtocol::UserAccountLoginMismatch,
                    clientReference, 0, loginBytes
                );
                return;
            }

            /* clean up state */
            _userAccountLoggingIn = "";
            _sessionSaltForUserLoggingIn.clear();

            QByteArray hashedPasswordFromClient =
                message.mid(12 + loginLength + userSaltLength + sessionSaltLength);

            bool loginSucceeded =
                Users::checkUserLoginPassword(
                    user, sessionSaltFromClient, hashedPasswordFromClient
                );

            if (loginSucceeded) {
                _userLoggedIn = user.id;
                _userLoggedInName = user.login;
                sendSuccessMessage(clientReference, user.id);
            }
            else {
                sendResultMessage(
                    NetworkProtocol::UserLoginAuthenticationFailed, clientReference, 0
                );
            }
        }
            break;
        default:
            qDebug() << "received unknown binary message type" << messageType
                     << " with length" << messageLength;
            break; /* unknown message type */
        }
    }

}
