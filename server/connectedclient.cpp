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

#include "common/hashid.h"
#include "common/networkutil.h"

#include "database.h"
#include "generator.h"
//#include "player.h"
#include "queue.h"
#include "queueentry.h"
#include "resolver.h"
#include "server.h"

namespace PMP {

    ConnectedClient::ConnectedClient(QTcpSocket* socket, Server* server, Player* player, Generator* generator)
     : QObject(server), _socket(socket), _server(server), _player(player), _generator(generator),
        _binaryMode(false), _clientProtocolNo(-1), _lastSentNowPlayingID(0)
    {
        connect(server, SIGNAL(shuttingDown()), this, SLOT(terminateConnection()));
        connect(socket, SIGNAL(disconnected()), this, SLOT(terminateConnection()));
        connect(socket, SIGNAL(readyRead()), this, SLOT(dataArrived()));
        connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(socketError(QAbstractSocket::SocketError)));
        connect(player, SIGNAL(volumeChanged(int)), this, SLOT(volumeChanged(int)));
        connect(generator, SIGNAL(enabledChanged(bool)), this, SLOT(dynamicModeStatusChanged(bool)));
        connect(generator, SIGNAL(noRepetitionSpanChanged(int)), this, SLOT(dynamicModeNoRepetitionSpanChanged(int)));
        connect(player, SIGNAL(stateChanged(Player::State)), this, SLOT(playerStateChanged(Player::State)));
        connect(player, SIGNAL(currentTrackChanged(QueueEntry const*)), this, SLOT(currentTrackChanged(QueueEntry const*)));
        connect(player, SIGNAL(positionChanged(qint64)), this, SLOT(trackPositionChanged(qint64)));
        connect(&player->queue(), SIGNAL(entryRemoved(quint32, quint32)), this, SLOT(queueEntryRemoved(quint32, quint32)));
        connect(&player->queue(), SIGNAL(entryAdded(quint32, quint32)), this, SLOT(queueEntryAdded(quint32, quint32)));

        /* send greeting */
        sendTextCommand("PMP 0.1 Welcome!");
    }

    void ConnectedClient::terminateConnection() {
        _socket->close();
        _textReadBuffer.clear();
        _socket->deleteLater();
        this->deleteLater();
    }

    void ConnectedClient::dataArrived() {
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
        while (!_binaryMode) {
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
        int spaceIndex = commandText.indexOf(' ');
        QString command = commandText;

        if (spaceIndex < 0) { /* command without arguments */
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

            }

            return;
        }

        /* split command at the space; don't include the space in the parts */
        QString rest = command.mid(spaceIndex + 1);
        command = command.left(spaceIndex);
        spaceIndex = rest.indexOf(' ');

        // one argument?
        if (spaceIndex < 0) {
            // 'volume' with one argument changes current volume
            if (command == "volume") {
                bool ok;
                uint volume = rest.toUInt(&ok);
                if (ok && volume >= 0 && volume <= 100) {
                    _player->setVolume(volume);
                }
            }
            else {
                /* unknown command ???? */

            }
        }

    }

    void ConnectedClient::sendTextCommand(QString const& command) {
        _socket->write((command + ";").toUtf8());
    }

    void ConnectedClient::sendBinaryMessage(QByteArray const& message) {
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
        NetworkUtil::append2Bytes(message, 1); /* message type */
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
        NetworkUtil::append2Bytes(message, 2); /* message type */
        NetworkUtil::appendByte(message, volume);

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendDynamicModeStatusMessage() {
        if (!_binaryMode) {
            return; // only supported in binary mode
        }

        quint8 enabled = _generator->enabled() ? 1 : 0;
        quint32 noRepetitionSpan = (quint32)_generator->noRepetitionSpan();

        QByteArray message;
        message.reserve(7);
        NetworkUtil::append2Bytes(message, 8); /* message type */
        NetworkUtil::appendByte(message, enabled);
        NetworkUtil::append4Bytes(message, noRepetitionSpan);

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
        NetworkUtil::append2Bytes(message, 5); /* message type */
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
        NetworkUtil::append2Bytes(message, 6); /* message type */
        NetworkUtil::append4Bytes(message, offset);
        NetworkUtil::append4Bytes(message, queueID);

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendQueueEntryAddedMessage(quint32 offset, quint32 queueID) {
        QByteArray message;
        message.reserve(10);
        NetworkUtil::append2Bytes(message, 7); /* message type */
        NetworkUtil::append4Bytes(message, offset);
        NetworkUtil::append4Bytes(message, queueID);

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendTrackInfoMessage(quint32 queueID) {
        QueueEntry* track = _player->queue().lookup(queueID);
        if (track == 0) { return; /* sorry, cannot send */ }

        track->checkTrackData(_player->resolver());

        QString title = track->title();
        QString artist = track->artist();
        qint32 length = track->lengthInSeconds(); /* SIGNED NUMBER!! */

        QByteArray titleData = title.toUtf8();
        QByteArray artistData = artist.toUtf8();

        QByteArray message;
        message.reserve((2 + 4 + 4 + 4 + 4) + titleData.size() + artistData.size());
        NetworkUtil::append2Bytes(message, 3); /* message type */
        NetworkUtil::append4Bytes(message, queueID);
        NetworkUtil::append4Bytes(message, length); /* length in seconds, SIGNED */
        NetworkUtil::append4Bytes(message, titleData.size());
        NetworkUtil::append4Bytes(message, artistData.size());
        message += titleData;
        message += artistData;

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendTrackInfoMessage(QList<quint32> const& queueIDs) {
        if (queueIDs.empty()) {
            return;
        }

        QByteArray message;
        message.reserve(2 + queueIDs.size() * (16 + /*title*/20 + /*artist*/15)); /* a guess at how much space we will need */
        NetworkUtil::append2Bytes(message, 4);

        quint32 queueID;
        foreach(queueID, queueIDs) {
            QueueEntry* track = _player->queue().lookup(queueID);
            if (track == 0) { continue; /* ID not found */ }

            track->checkTrackData(_player->resolver());

            QString title = track->title();
            QString artist = track->artist();
            qint32 length = track->lengthInSeconds(); /* SIGNED NUMBER!! */

            QByteArray titleData = title.toUtf8();
            QByteArray artistData = artist.toUtf8();

            NetworkUtil::append4Bytes(message, queueID);
            NetworkUtil::append4Bytes(message, length); /* length in seconds, SIGNED */
            NetworkUtil::append4Bytes(message, titleData.size());
            NetworkUtil::append4Bytes(message, artistData.size());
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
        NetworkUtil::append2Bytes(message, 9);

        NetworkUtil::append4Bytes(message, queueID);

        Q_FOREACH(QString name, names) {
            QByteArray nameBytes = name.toUtf8();
            NetworkUtil::append4Bytes(message, nameBytes.size());
            message += nameBytes;
        }

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
        HashID const* hash = entry->hash();

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

    void ConnectedClient::readBinaryCommands() {
        char lengthBytes[4];

        while (_socket->peek(lengthBytes, sizeof(lengthBytes)) == sizeof(lengthBytes)) {
            quint32 messageLength = NetworkUtil::get4Bytes(lengthBytes);

            if (_socket->bytesAvailable() - sizeof(lengthBytes) < messageLength) {
                qDebug() << "waiting for incoming message with length" << messageLength << " --- only partially received";
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

        switch (messageType) {
        case 1: /* single-byte action message */
        {
            if (messageLength != 3) {
                return; /* invalid message */
            }

            quint8 actionType = NetworkUtil::getByte(message, 2);

            if (actionType >= 100 && actionType <= 200) {
                qDebug() << "received CHANGE VOLUME command, volume" << ((int)actionType - 100);
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
            case 10: /* request for state info */
                qDebug() << "received request for player status";
                sendStateInfo();
                break;
            case 11: /* request for status of dynamic mode */
                qDebug() << "received request for dynamic mode status";
                sendDynamicModeStatusMessage();
                break;
            case 20: /* enable dynamic mode */
                qDebug() << "received ENABLE DYNAMIC MODE command";
                _generator->enable();
                break;
            case 21: /* disable dynamic mode */
                qDebug() << "received DISABLE DYNAMIC MODE command";
                _generator->disable();
                break;
            case 99:
                qDebug() << "received SHUTDOWN command";
                _server->shutdown();
                break;
            default:
                qDebug() << "received unrecognized single-byte action type:" << (int)actionType;
                break; /* unknown action type */
            }
        }
            break;
        case 2: /* request for track info by QID */
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
        case 3: /* bulk request for track info by QID */
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
        case 4: /* queue fetch request */
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
        case 5: /* request to delete an entry in the queue */
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
        case 6: /* request to change generator non-repetition interval */
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
        case 7: /* request for possible filenames of a QID */
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

            const HashID* hash = entry->hash();
            if (hash == 0) {
                /* hash not found */
                /* TODO: register callback to resume this once the hash becomes known */
                return;
            }

            uint hashID = _player->resolver().getID(*hash);

            /* FIXME: do this in another thread, it will slow down the main thread */

            Database* db = Database::instance();
            if (db == 0) {
                return; /* database unusable */
            }

            QList<QString> filenames = db->getFilenames(hashID);

            sendPossibleTrackFilenames(queueID, filenames);
        }
            break;
        case 8: /* seek command */
        {
            if (messageLength != 14) {
                return; /* invalid message */
            }

            quint32 queueID = NetworkUtil::get4Bytes(message, 2);
            qint64 position = (qint64)NetworkUtil::get8Bytes(message, 6);

            qDebug() << "received seek command; QID:" << queueID << "  position:" << position;

            if (queueID != _player->nowPlayingQID()) {
                return; /* invalid queue ID */
            }

            if (position < 0) return; /* invalid position */

            _player->seekTo(position);
        }
        default:
            qDebug() << "received unknown binary message type" << messageType << " with length" << messageLength;
            break; /* unknown message type */
        }
    }

}
