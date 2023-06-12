/*
    Copyright (C) 2014-2023, Kevin Andre <hyperquantum@gmail.com>

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
#include "common/version.h"

#include "collectionmonitor.h"
#include "player.h"
#include "playerqueue.h"
#include "queueentry.h"
#include "resolver.h"
#include "scrobbling.h"
#include "serverhealthmonitor.h"
#include "serverinterface.h"
#include "users.h"

#include <QMap>
#include <QThreadPool>
#include <QTimer>

namespace PMP::Server
{
    /* ====================== ConnectedClient ====================== */

    const qint16 ConnectedClient::ServerProtocolNo = 23;

    ConnectedClient::ConnectedClient(QTcpSocket* socket, ServerInterface* serverInterface,
                                     Player* player,
                                     Users* users,
                                     CollectionMonitor* collectionMonitor,
                                     ServerHealthMonitor* serverHealthMonitor,
                                     Scrobbling* scrobbling)
     : _socket(socket), _serverInterface(serverInterface),
       _player(player),
       _users(users), _collectionMonitor(collectionMonitor),
       _serverHealthMonitor(serverHealthMonitor),
       _scrobbling(scrobbling),
       _clientProtocolNo(-1),
       _lastSentNowPlayingID(0),
       _terminated(false),
       _binaryMode(false),
       _eventsEnabled(false),
       _healthEventsEnabled(false),
       _pendingPlayerStatus(false)
    {
        _serverInterface->setParent(this);

        _extensionsThis.registerExtensionSupport({NetworkProtocolExtension::Scrobbling,
                                                  222, 2});

        connect(
            _serverInterface, &ServerInterface::serverShuttingDown,
            this,
            [this]()
            {
                qDebug() << "server shutting down; terminating this connection";
                terminateConnection();
            }
        );

        connect(
            socket, &QTcpSocket::disconnected,
            this,
            [this]()
            {
                if (_terminated) return;

                qDebug() << "TCP socket disconnected; terminating this connection";
                terminateConnection();
            }
        );
        connect(socket, &QTcpSocket::readyRead, this, &ConnectedClient::dataArrived);
        connect(socket, &QTcpSocket::errorOccurred, this, &ConnectedClient::socketError);

        /* Send greeting.
         * The space at the end allows the client to detect that this server supports the
         * new 'binary' text command with one argument. A server whose greeting ends with
         * "Welcome!" without space will likely only support the 'binary' command without
         * argument. Of course, the greeting could also be changed to something completely
         * different from "Welcome!", in which case the space at the end will no longer be
         * needed. */
        sendTextCommand("PMP " PMP_VERSION_DISPLAY " Welcome! ");
    }

    ConnectedClient::~ConnectedClient()
    {
        qDebug() << "ConnectedClient: destructor called";
        _socket->deleteLater();
    }

    void ConnectedClient::terminateConnection()
    {
        qDebug() << "terminateConnection() called";
        if (_terminated) return;
        qDebug() << " will terminate and clean up connection now";
        _terminated = true;
        _socket->close();
        _textReadBuffer.clear();
        this->deleteLater();
    }

    void ConnectedClient::enableEvents()
    {
        if (_eventsEnabled)
            return;

        qDebug() << "enabling event notifications";

        enableHealthEvents(GeneralOrSpecific::General);

        _eventsEnabled = true;

        auto queue = &_player->queue();
        auto resolver = &_player->resolver();

        connect(_player, &Player::volumeChanged, this, &ConnectedClient::volumeChanged);
        connect(
            _player, &Player::stateChanged,
            this, &ConnectedClient::playerStateChanged
        );
        connect(
            _player, &Player::currentTrackChanged,
            this, &ConnectedClient::currentTrackChanged
        );
        connect(
            _player, &Player::newHistoryEntry,
            this, &ConnectedClient::newHistoryEntry
        );
        connect(
            _player, &Player::positionChanged,
            this, &ConnectedClient::trackPositionChanged
        );
        connect(
            _player, &Player::userPlayingForChanged,
            this, &ConnectedClient::onUserPlayingForChanged
        );

        connect(
            _serverInterface, &ServerInterface::delayedStartActiveChanged,
            this, &ConnectedClient::onDelayedStartActiveChanged
        );

        connect(
            _serverInterface, &ServerInterface::dynamicModeStatusEvent,
            this, &ConnectedClient::onDynamicModeStatusEvent
        );
        connect(
            _serverInterface, &ServerInterface::dynamicModeWaveStatusEvent,
            this, &ConnectedClient::onDynamicModeWaveStatusEvent
        );

        connect(
            queue, &PlayerQueue::entryRemoved,
            this, &ConnectedClient::queueEntryRemoved
        );
        connect(
            _serverInterface, &ServerInterface::queueEntryAddedWithoutReference,
            this, &ConnectedClient::queueEntryAddedWithoutReference
        );
        connect(
            _serverInterface, &ServerInterface::queueEntryAddedWithReference,
            this, &ConnectedClient::queueEntryAddedWithReference
        );
        connect(
            queue, &PlayerQueue::entryMoved,
            this, &ConnectedClient::queueEntryMoved
        );

        connect(
            resolver, &Resolver::fullIndexationRunStatusChanged,
            this, &ConnectedClient::onFullIndexationRunStatusChanged
        );

        connect(
            _collectionMonitor, &CollectionMonitor::hashAvailabilityChanged,
            this, &ConnectedClient::onHashAvailabilityChanged
        );
        connect(
            _collectionMonitor, &CollectionMonitor::hashInfoChanged,
            this, &ConnectedClient::onHashInfoChanged
        );

        connect(
            _serverInterface, &ServerInterface::hashUserDataChangedOrAvailable,
            this,
            [this](quint32 userId, QVector<HashStats> stats)
            {
                sendHashUserDataMessage(userId, stats);
            }
        );

        connect(
            _serverInterface, &ServerInterface::serverCaptionChanged,
            this, [this]() { sendServerNameMessage(); }
        );

        connect(
            _serverInterface, &ServerInterface::serverClockTimeSendingPulse,
            this, &ConnectedClient::sendServerClockMessage
        );
        sendServerClockMessage();
    }

    void ConnectedClient::enableHealthEvents(GeneralOrSpecific howEnabled)
    {
        auto healthEventsWereEnabledAlready = _eventsEnabled | _healthEventsEnabled;

        if (howEnabled == GeneralOrSpecific::Specific)
            _healthEventsEnabled = true;

        if (healthEventsWereEnabledAlready)
            return; /* nothing to do */

        connect(
            _serverHealthMonitor, &ServerHealthMonitor::serverHealthChanged,
            this, &ConnectedClient::serverHealthChanged
        );

        sendServerHealthMessageIfNotEverythingOkay();
    }

    bool ConnectedClient::isLoggedIn() const
    {
        return _serverInterface->isLoggedIn();
    }

    void ConnectedClient::connectSlotsAfterSuccessfulUserLogin(quint32 userLoggedIn)
    {
        auto userScrobblingController = _scrobbling->getControllerForUser(userLoggedIn);

        connect(
            userScrobblingController, &UserScrobblingController::scrobblingProviderInfo,
            this, &ConnectedClient::onScrobblingProviderInfo
        );

        connect(
            userScrobblingController, &UserScrobblingController::scrobblerStatusChanged,
            this, &ConnectedClient::onScrobblerStatusChanged
        );

        connect(
            userScrobblingController,
            &UserScrobblingController::scrobblingProviderEnabledChanged,
            this, &ConnectedClient::onScrobblingProviderEnabledChanged
        );
    }

    void ConnectedClient::dataArrived()
    {
        if (_terminated)
        {
            qDebug() << "dataArrived called on a terminated connection???";
            return;
        }

        if (_binaryMode && _clientProtocolNo >= 0)
        {
            readBinaryCommands();
            return;
        }

        if (!_binaryMode)
        {
            /* textual command mode */
            readTextCommands();
        }

        /* not changed to binary mode? */
        if (!_binaryMode) { return; }

        /* do we still need to read the binary header? */
        if (_clientProtocolNo < 0)
        {
            if (_socket->bytesAvailable() < 5)
                return; /* not enough data */

            char heading[5];
            _socket->read(heading, 5);

            if (heading[0] != 'P'
                || heading[1] != 'M'
                || heading[2] != 'P')
            {
                qDebug() << "incoming connection without PMP signature; terminating it";
                terminateConnection();
                return;
            }

            _clientProtocolNo = (heading[3] << 8) + heading[4];
            qDebug() << "client protocol version:" << _clientProtocolNo;

            /* Help out old clients that do not know that the client now has to explicitly
             * ask for event notifications. */
            if (_clientProtocolNo < 2)
                enableEvents();
        }

        readBinaryCommands();
    }

    void ConnectedClient::socketError(QAbstractSocket::SocketError error)
    {
        switch (error)
        {
        case QAbstractSocket::RemoteHostClosedError:
            qDebug() << "got socket error RemoteHostClosedError; terminating connection";
            terminateConnection();
            break;
        default:
            qDebug() << "ConnectedClient: unhandled socket error:" << error;
            break;
        }
    }

    void ConnectedClient::readTextCommands()
    {
        while (!_terminated && !_binaryMode)
        {
            bool hadSemicolon = false;
            char c;
            while (_socket->getChar(&c))
            {
                if (c == ';')
                {
                    hadSemicolon = true;
                    /* skip an optional newline after the semicolon */
                    if (_socket->peek(&c, 1) > 0 && c == '\r') _socket->read(&c, 1);
                    if (_socket->peek(&c, 1) > 0 && c == '\n') _socket->read(&c, 1);
                    break;
                }

                _textReadBuffer.append(c); /* not for semicolons */
            }

            if (!hadSemicolon)
                break; /* no complete text command in received data */

            QString commandString = QString::fromUtf8(_textReadBuffer);

            _textReadBuffer.clear(); /* text was consumed completely */

            executeTextCommand(commandString);
        }
    }

    void ConnectedClient::executeTextCommandWithoutArgs(const QString& command)
    {
        if (command == "play")
        {
            _serverInterface->play();
        }
        else if (command == "pause")
        {
            _serverInterface->pause();
        }
        else if (command == "skip")
        {
            _serverInterface->skip();
        }
        else if (command == "volume")
        {
            /* 'volume' without arguments sends current volume */
            sendVolumeMessage();
        }
        else if (command == "state")
        {
            /* pretend state has changed, in order to send state info */
            playerStateChanged(_player->state());
        }
        else if (command == "nowplaying")
        {
            /* pretend current track has changed, in order to send current track info */
            currentTrackChanged(_player->nowPlaying());
        }
        else if (command == "queue")
        {
            sendTextualQueueInfo();
        }
        else if (command == "shutdown")
        {
            _serverInterface->shutDownServer();
        }
        else if (command == "binary")
        {
            /* 'binary' command without argument is obsolete, but we need to continue
             * supporting it for some time in order to be compatible with older clients */
            qDebug() << " 'binary' command has obsolete form (needs argument)";
            handleBinaryModeSwitchRequest();
        }
        else
        {
            /* unknown command ???? */
            qDebug() << " unknown text command: " << command;
        }
    }

    void ConnectedClient::executeTextCommandWithArgs(const QString& command,
                                                     const QString& arg1)
    {
        if (command == "binary")
        {
            /* Requiring an argument that is (nearly) impossible to remember will prevent
             * an accidental switch to binary mode when a person is typing commands. */
            if (arg1 == "NUxwyGR3ivTcB27VGYdy")
            {
                handleBinaryModeSwitchRequest();
            }
            else
            {
                qDebug() << " argument for 'binary' command not recognized";
            }
        }
        else if (command == "volume")
        {
            /* 'volume' with one argument is a request to changes the current volume */
            bool ok;
            int volume = arg1.toInt(&ok);
            if (ok && volume >= 0 && volume <= 100)
            {
                _serverInterface->setVolume(volume);
            }
        }
        else if (command == "shutdown")
        {
            _serverInterface->shutDownServer(arg1);
        }
        else
        {
            /* unknown command ???? */
            qDebug() << " unknown text command: " << command;
        }
    }

    void ConnectedClient::executeTextCommandWithArgs(const QString& command,
                                                     const QString& arg1,
                                                     const QString& arg2)
    {
        if (command == "qmove")
        {
            bool ok;
            uint queueID = arg1.toUInt(&ok);
            if (!ok || queueID == 0) return;

            if (!arg2.startsWith("+") && !arg2.startsWith("-")) return;
            int moveDiff = arg2.toInt(&ok);
            if (!ok || moveDiff == 0) return;

            _serverInterface->moveQueueEntry(queueID, moveDiff);
        }
        else
        {
            /* unknown command ???? */
            qDebug() << " unknown text command: " << command;
        }
    }

    void ConnectedClient::executeTextCommand(QString const& commandText)
    {
        qDebug() << "received text commandline:" << commandText;

        int spaceIndex = commandText.indexOf(' ');

        if (spaceIndex < 0) /* command without arguments */
        {
            executeTextCommandWithoutArgs(commandText);
            return;
        }

        /* split command at the space; don't include the space in the parts */
        QString command = commandText.left(spaceIndex);
        QString rest = commandText.mid(spaceIndex + 1);
        spaceIndex = rest.indexOf(' ');

        if (spaceIndex < 0) /* one argument only */
        {
            executeTextCommandWithArgs(command, rest);
            return;
        }

        /* two arguments or more */

        QString arg1 = rest.left(spaceIndex);
        rest = rest.mid(spaceIndex + 1);
        spaceIndex = rest.indexOf(' ');

        if (spaceIndex < 0) /* two arguments only */
        {
            executeTextCommandWithArgs(command, arg1, rest);
            return;
        }

        /* unknown command ???? */
        qDebug() << " unknown text command, or wrong number of arguments";
    }

    void ConnectedClient::sendTextCommand(QString const& command)
    {
        if (!_socket->isValid())
        {
            qWarning() << "cannot send text command when socket not in valid state";
            return;
        }

        if (_terminated)
        {
            qWarning() << "cannot send text command because connection is terminated";
            return;
        }

        _socket->write((command + ";").toUtf8());
        _socket->flush();
    }

    void ConnectedClient::handleBinaryModeSwitchRequest()
    {
        if (_binaryMode)
        {
            qDebug() << "cannot switch to binary mode, already in it";
            return;
        }

        /* switch to binary mode */
        _binaryMode = true;
        /* tell the client that all further communication will be in binary mode */
        sendTextCommand("binary");

        /* send binary hello */
        QByteArray binaryHeader;
        binaryHeader.reserve(5);
        binaryHeader.append("PMP", 3);
        NetworkUtil::append2Bytes(binaryHeader, ServerProtocolNo);
        _socket->write(binaryHeader);
        _socket->flush();
    }

    void ConnectedClient::appendScrobblingMessageStart(QByteArray& buffer,
                                                  ScrobblingServerMessageType messageType)
    {
        auto type = static_cast<quint8>(messageType);

        buffer +=
            NetworkProtocolExtensionMessages::generateExtensionMessageStart(
                NetworkProtocolExtension::Scrobbling, _extensionsThis, type
            );
    }

    void ConnectedClient::sendBinaryMessage(QByteArray const& message)
    {
        if (!_socket->isValid())
        {
            qWarning() << "cannot send binary message when socket not in valid state";
            return;
        }

        if (_terminated)
        {
            qWarning() << "cannot send binary message because connection is terminated";
            return;
        }

        if (!_binaryMode)
        {
            qWarning() << "cannot send binary message when not in binary mode";
            return; /* only supported in binary mode */
        }

        auto messageLength = message.length();
        if (messageLength > std::numeric_limits<qint32>::max() - 1)
        {
            qWarning() << "Message too long for sending; length:" << messageLength;
            return;
        }

        QByteArray lengthBytes;
        NetworkUtil::append4BytesSigned(lengthBytes, messageLength);

        _socket->write(lengthBytes);
        _socket->write(message);
        _socket->flush();
    }

    void ConnectedClient::sendKeepAliveReply(quint8 blob)
    {
        QByteArray message;
        message.reserve(4);
        NetworkProtocol::append2Bytes(message, ServerMessageType::KeepAliveMessage);
        NetworkUtil::append2Bytes(message, blob);

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendProtocolExtensionsMessage()
    {
        if (_clientProtocolNo < 12) return; /* client will not understand this message */

        auto message =
            NetworkProtocolExtensionMessages::generateExtensionSupportMessage(
                ClientOrServer::Server, _extensionsThis
            );

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendStateInfoAfterTimeout()
    {
        _pendingPlayerStatus = false;
        sendPlayerStateMessage();
    }

    void ConnectedClient::sendPlayerStateMessage()
    {
        //qDebug() << "sending state info";

        auto playerStateOverview = _serverInterface->getPlayerStateOverview();

        quint8 stateNum = 0;
        switch (playerStateOverview.playerState)
        {
        case ServerPlayerState::Stopped:
            stateNum = 1;
            break;
        case ServerPlayerState::Playing:
            stateNum = 2;
            break;
        case ServerPlayerState::Paused:
            stateNum = 3;
            break;
        }

        if (_clientProtocolNo >= 20 && playerStateOverview.delayedStartActive)
            stateNum |= 128;

        QByteArray message;
        message.reserve(20);
        NetworkProtocol::append2Bytes(message, ServerMessageType::PlayerStateMessage);
        NetworkUtil::appendByte(message, stateNum);
        NetworkUtil::appendByte(message, playerStateOverview.volume);
        NetworkUtil::append4Bytes(message, playerStateOverview.queueLength);
        NetworkUtil::append4Bytes(message, playerStateOverview.nowPlayingQueueId);
        NetworkUtil::append8Bytes(message, playerStateOverview.trackPosition);

        sendBinaryMessage(message);

        _lastSentNowPlayingID = playerStateOverview.nowPlayingQueueId;
    }

    void ConnectedClient::sendVolumeMessage()
    {
        quint8 volume = _player->volume();

        if (!_binaryMode)
        {
            sendTextCommand("volume " + QString::number(volume));
            return;
        }

        QByteArray message;
        message.reserve(3);
        NetworkProtocol::append2Bytes(message, ServerMessageType::VolumeChangedMessage);
        NetworkUtil::appendByte(message, volume);

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendDynamicModeStatusMessage(StartStopEventStatus enabledStatus,
                                                       int noRepetitionSpanSeconds)
    {
        quint8 enabled = Common::isActive(enabledStatus) ? 1 : 0;
        qint32 noRepetitionSpan = noRepetitionSpanSeconds;

        QByteArray message;
        message.reserve(7);
        NetworkProtocol::append2Bytes(message,
                                      ServerMessageType::DynamicModeStatusMessage);
        NetworkUtil::appendByte(message, enabled);
        NetworkUtil::append4BytesSigned(message, noRepetitionSpan);

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendUserPlayingForModeMessage()
    {
        quint32 user = _player->userPlayingFor();
        QString login = _users->getUserLogin(user);
        QByteArray loginBytes = login.toUtf8();

        QByteArray message;
        message.reserve(4 + 4 + loginBytes.length());
        NetworkProtocol::append2Bytes(message,
                                      ServerMessageType::UserPlayingForModeMessage);
        NetworkUtil::appendByte(message, quint8(loginBytes.size()));
        NetworkUtil::appendByte(message, 0); /* unused */
        NetworkUtil::append4Bytes(message, user);
        message += loginBytes;

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendGeneratorWaveStatusMessage(StartStopEventStatus status,
                                                         quint32 user,
                                                         int waveDeliveredCount,
                                                         int waveTotalCount)
    {
        if (_clientProtocolNo < 8) return; /* client will not understand this message */

        QByteArray message;
        message.reserve(8 + 4);
        NetworkProtocol::append2Bytes(message,
                                      ServerMessageType::DynamicModeWaveStatusMessage);
        NetworkUtil::appendByte(message, 0); /* unused */
        NetworkUtil::appendByte(message, quint8(status));
        NetworkUtil::append4Bytes(message, user);
        if (_clientProtocolNo >= 14)
        {
            bool error = false;
            qint16 progress =
                NetworkUtil::to2BytesSigned(waveDeliveredCount, error, "wave progress");
            qint16 progressTotal =
                NetworkUtil::to2BytesSigned(waveTotalCount, error, "wave progress total");

            NetworkUtil::append2BytesSigned(message, progress);
            NetworkUtil::append2BytesSigned(message, progressTotal);
        }

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendEventNotificationMessage(ServerEventCode eventCode)
    {
        qDebug() << "sending server event number" << static_cast<int>(eventCode);

        quint8 numericEventCode = static_cast<quint8>(eventCode);

        QByteArray message;
        message.reserve(2 + 2);
        NetworkProtocol::append2Bytes(message,
                                      ServerMessageType::ServerEventNotificationMessage);
        NetworkUtil::appendByte(message, numericEventCode);
        NetworkUtil::appendByte(message, 0); /* unused */

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendServerVersionInfoMessage()
    {
        auto versionInfo = _serverInterface->getServerVersionInfo();

        auto programNameBytes = versionInfo.programName.left(63).toUtf8();
        auto versionDisplayBytes = versionInfo.versionForDisplay.left(63).toUtf8();
        auto vcsBuildBytes = versionInfo.vcsBuild.left(63).toUtf8();
        auto vcsBranchBytes = versionInfo.vcsBranch.left(63).toUtf8();

        QByteArray message;
        message.reserve(2 + 2 + 4
                        + programNameBytes.size() + versionDisplayBytes.size()
                        + vcsBuildBytes.size() + vcsBranchBytes.size());
        NetworkProtocol::append2Bytes(message,
                                      ServerMessageType::ServerVersionInfoMessage);
        NetworkUtil::append2Bytes(message, 0); // filler
        NetworkUtil::appendByteUnsigned(message, programNameBytes.size());
        NetworkUtil::appendByteUnsigned(message, versionDisplayBytes.size());
        NetworkUtil::appendByteUnsigned(message, vcsBuildBytes.size());
        NetworkUtil::appendByteUnsigned(message, vcsBranchBytes.size());
        message += programNameBytes;
        message += versionDisplayBytes;
        message += vcsBuildBytes;
        message += vcsBranchBytes;

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendServerInstanceIdentifier()
    {
        QUuid uuid = _serverInterface->getServerUuid();

        QByteArray message;
        message.reserve(2 + 16);
        NetworkProtocol::append2Bytes(message,
                                      ServerMessageType::ServerInstanceIdentifierMessage);
        message.append(uuid.toRfc4122());

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendDatabaseIdentifier()
    {
        auto maybeUuid = _serverInterface->getDatabaseUuid();

        if (maybeUuid.failed())
            return;

        auto uuid = maybeUuid.result();

        QByteArray message;
        message.reserve(2 + 16);
        NetworkProtocol::append2Bytes(message,
                                      ServerMessageType::DatabaseIdentifierMessage);
        message.append(uuid.toRfc4122());

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendUsersList()
    {
        auto const users = _users->getUsers();
        auto usersCount = users.size();
        if (usersCount > std::numeric_limits<quint16>::max())
        {
            qWarning() << "users count exceeds limit, cannot send list";
            return;
        }

        QByteArray message;
        message.reserve(2 + 2 + users.size() * (4 + 1 + 20)); /* only an approximation */
        NetworkProtocol::append2Bytes(message, ServerMessageType::UsersListMessage);
        NetworkUtil::append2Bytes(message, static_cast<quint16>(usersCount));

        for (auto& user : users)
        {
            NetworkUtil::append4Bytes(message, user.first); /* ID */

            QByteArray loginNameBytes = user.second.toUtf8();
            loginNameBytes.truncate(255);

            NetworkUtil::appendByte(message, static_cast<quint8>(loginNameBytes.size()));
            message += loginNameBytes;
        }

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendQueueContentMessage(qint32 startOffset, quint8 length)
    {
        PlayerQueue& queue = _player->queue();
        int queueLength = queue.length();

        if (startOffset >= queueLength)
        {
            length = 0;
        }
        else if (startOffset > queueLength - length)
        {
            length = queueLength - startOffset;
        }

        auto entries = queue.entries(startOffset, (length == 0) ? -1 : length);

        QByteArray message;
        message.reserve(10 + entries.length() * 4);
        NetworkProtocol::append2Bytes(message, ServerMessageType::QueueContentsMessage);
        NetworkUtil::append4Bytes(message, queueLength);
        NetworkUtil::append4Bytes(message, startOffset);

        for (auto& entry : entries)
        {
            NetworkUtil::append4Bytes(message, entry->queueID());
        }

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendQueueHistoryMessage(int limit)
    {
        PlayerQueue& queue = _player->queue();

        /* keep a reasonable limit */
        if (limit <= 0 || limit > 255) { limit = 255; }

        const auto entries = queue.recentHistory(limit);
        auto entryCount = static_cast<quint8>(entries.length());

        QByteArray message;
        message.reserve(2 + 1 + 1 + entryCount * 28);
        NetworkProtocol::append2Bytes(message, ServerMessageType::PlayerHistoryMessage);
        NetworkUtil::appendByte(message, 0); /* filler */
        NetworkUtil::appendByte(message, entryCount);

        for (auto& entry : entries)
        {
            quint16 status =
                (entry->hadError() ? 1 : 0) | (entry->hadSeek() ? 2 : 0);

            qint16 permillage = static_cast<qint16>(entry->permillage());

            NetworkUtil::append4Bytes(message, entry->queueID());
            NetworkUtil::append4Bytes(message, entry->user());
            NetworkUtil::append8ByteQDateTimeMsSinceEpoch(message, entry->started());
            NetworkUtil::append8ByteQDateTimeMsSinceEpoch(message, entry->ended());
            NetworkUtil::append2BytesSigned(message, permillage);
            NetworkUtil::append2Bytes(message, status);
        }

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendHashUserDataMessage(quint32 userId,
                                                  QVector<HashStats> stats)
    {
        const int maxSize = (1 << 16) - 1;

        /* not too big? */
        if (stats.size() > maxSize)
        {
            /* TODO: maybe delay the second part? */
            sendHashUserDataMessage(userId, stats.mid(0, maxSize));
            sendHashUserDataMessage(userId, stats.mid(maxSize));
            return;
        }

        qint16 fields = 1 /* previously heard */ | 2 /* score */;
        fields &=
            NetworkProtocol::getHashUserDataFieldsMaskForProtocolVersion(
                                                                       _clientProtocolNo);
        bool sendPreviouslyHeard = fields & 1;
        bool sendScore = fields & 2;
        uint fieldsSize = (sendPreviouslyHeard ? 8 : 0) + (sendScore ? 2 : 0);

        qDebug() << "sending user track data for" << stats.size()
                 << "hashes; fields:" << fields;

        QByteArray message;
        message.reserve(
            2 + 2 + 4 + 4
                + stats.size() * (NetworkProtocol::FILEHASH_BYTECOUNT + fieldsSize)
        );

        NetworkProtocol::append2Bytes(message, ServerMessageType::HashUserDataMessage);
        NetworkUtil::append2Bytes(message, stats.size());
        NetworkUtil::append2Bytes(message, 0); /* filler */
        NetworkUtil::append2Bytes(message, fields);
        NetworkUtil::append4Bytes(message, userId);

        for (auto& stat : qAsConst(stats))
        {
            NetworkProtocol::appendHash(message, stat.hash());

            if (sendPreviouslyHeard)
            {
                NetworkUtil::append8ByteMaybeEmptyQDateTimeMsSinceEpoch(
                    message, stat.stats().lastHeard()
                );
            }

            if (sendScore)
            {
                NetworkUtil::append2Bytes(message, (quint16)stat.stats().score());
            }
        }

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendQueueEntryRemovedMessage(qint32 offset, quint32 queueID)
    {
        QByteArray message;
        message.reserve(10);
        NetworkProtocol::append2Bytes(message,
                                      ServerMessageType::QueueEntryRemovedMessage);
        NetworkUtil::append4Bytes(message, offset);
        NetworkUtil::append4Bytes(message, queueID);

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendQueueEntryAddedMessage(qint32 offset, quint32 queueID)
    {
        QByteArray message;
        message.reserve(10);
        NetworkProtocol::append2Bytes(message, ServerMessageType::QueueEntryAddedMessage);
        NetworkUtil::append4Bytes(message, offset);
        NetworkUtil::append4Bytes(message, queueID);

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendQueueEntryAdditionConfirmationMessage(
            quint32 clientReference, qint32 index, quint32 queueID)
    {
        QByteArray message;
        message.reserve(16);
        NetworkProtocol::append2Bytes(message,
                                ServerMessageType::QueueEntryAdditionConfirmationMessage);
        NetworkUtil::append2Bytes(message, 0); /* filler */
        NetworkUtil::append4Bytes(message, clientReference);
        NetworkUtil::append4Bytes(message, index);
        NetworkUtil::append4Bytes(message, queueID);

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendQueueEntryMovedMessage(qint32 fromOffset, qint32 toOffset,
                                                     quint32 queueID)
    {
        QByteArray message;
        message.reserve(14);
        NetworkProtocol::append2Bytes(message, ServerMessageType::QueueEntryMovedMessage);
        NetworkUtil::append4Bytes(message, fromOffset);
        NetworkUtil::append4Bytes(message, toOffset);
        NetworkUtil::append4Bytes(message, queueID);

        sendBinaryMessage(message);
    }

    quint16 ConnectedClient::createTrackStatusFor(QSharedPointer<QueueEntry> entry)
    {
        switch (entry->kind())
        {
        case QueueEntryKind::Track:
            break; /* handled below */

        case QueueEntryKind::Break:
            return NetworkProtocol::createTrackStatusFor(SpecialQueueItemType::Break);

        case QueueEntryKind::Barrier:
            return NetworkProtocol::createTrackStatusFor(SpecialQueueItemType::Barrier);
        }

        if (!entry->isTrack()) /* unhandled non-track thing */
        {
            qWarning() << "Unhandled QueueEntryKind" << int(entry->kind());
            return NetworkProtocol::createTrackStatusForUnknownThing();
        }

        return NetworkProtocol::createTrackStatusForTrack();
    }

    void ConnectedClient::sendQueueEntryInfoMessage(quint32 queueID)
    {
        auto track = _player->queue().lookup(queueID);
        if (track == nullptr) { return; /* sorry, cannot send */ }

        track->checkTrackData(_player->resolver());

        bool preciseLength = _clientProtocolNo >= 13;

        auto trackStatus = createTrackStatusFor(track);
        QString title = track->title();
        QString artist = track->artist();
        qint64 lengthMilliseconds = track->lengthInMilliseconds();

        const int maxSize = (1 << 16) - 1;

        /* worst case: 4 bytes in UTF-8 for each char */
        title.truncate(maxSize / 4);
        artist.truncate(maxSize / 4);

        QByteArray titleData = title.toUtf8();
        QByteArray artistData = artist.toUtf8();

        QByteArray message;
        message.reserve(2 + 2 + 4 + 8 + 2 + 2 + titleData.size() + artistData.size());
        NetworkProtocol::append2Bytes(message, ServerMessageType::TrackInfoMessage);
        NetworkUtil::append2Bytes(message, trackStatus);
        NetworkUtil::append4Bytes(message, queueID);

        if (preciseLength)
            NetworkUtil::append8BytesSigned(message, lengthMilliseconds);
        else
            NetworkUtil::append4BytesSigned(
                        message,
                        lengthMilliseconds < 0
                            ? -1
                            : lengthMilliseconds / 1000);

        NetworkUtil::append2Bytes(message, titleData.size());
        NetworkUtil::append2Bytes(message, artistData.size());
        message += titleData;
        message += artistData;

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendQueueEntryInfoMessage(QList<quint32> const& queueIDs)
    {
        if (queueIDs.empty())
            return;

        const int maxSize = (1 << 16) - 1;

        /* not too big? */
        if (queueIDs.size() > maxSize)
        {
            /* TODO: maybe delay the second part? */
            sendQueueEntryInfoMessage(queueIDs.mid(0, maxSize));
            sendQueueEntryInfoMessage(queueIDs.mid(maxSize));
            return;
        }

        bool preciseLength = _clientProtocolNo >= 13;

        QByteArray message;
        /* reserve some memory, take a guess at how much space we will probably need */
        message.reserve(
            4 + queueIDs.size() * (2 + 8 + 8 + /*title*/20 + /*artist*/15)
        );

        NetworkProtocol::append2Bytes(message, ServerMessageType::BulkTrackInfoMessage);
        NetworkUtil::append2Bytes(message, (uint)queueIDs.size());

        PlayerQueue& queue = _player->queue();

        /* TODO: bug: concurrency issue here when a QueueEntry has just been deleted */

        for(quint32 queueID : queueIDs)
        {
            auto track = queue.lookup(queueID);
            auto trackStatus =
                track ? createTrackStatusFor(track)
                      : NetworkProtocol::createTrackStatusUnknownId();
            NetworkUtil::append2Bytes(message, trackStatus);
        }
        if (queueIDs.size() % 2 != 0)
        {
            NetworkUtil::append2Bytes(message, 0); /* filler */
        }

        for(quint32 queueID : queueIDs)
        {
            auto track = queue.lookup(queueID);
            if (track == 0) { continue; /* ID not found */ }

            track->checkTrackData(_player->resolver());

            QString title = track->title();
            QString artist = track->artist();
            qint64 lengthMilliseconds = track->lengthInMilliseconds();

            /* worst case: 4 bytes in UTF-8 for each char */
            title.truncate(maxSize / 4);
            artist.truncate(maxSize / 4);

            QByteArray titleData = title.toUtf8();
            QByteArray artistData = artist.toUtf8();

            NetworkUtil::append4Bytes(message, queueID);

            if (preciseLength)
                NetworkUtil::append8BytesSigned(message, lengthMilliseconds);
            else
                NetworkUtil::append4BytesSigned(
                            message,
                            lengthMilliseconds < 0
                                ? -1
                                : lengthMilliseconds / 1000);

            NetworkUtil::append2Bytes(message, (uint)titleData.size());
            NetworkUtil::append2Bytes(message, (uint)artistData.size());
            message += titleData;
            message += artistData;
        }

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendQueueEntryHashMessage(const QList<quint32>& queueIDs)
    {
        if (queueIDs.empty())
            return;

        const int maxSize = (1 << 16) - 1;

        /* not too big? */
        if (queueIDs.size() > maxSize)
        {
            /* TODO: maybe delay the second part? */
            sendQueueEntryInfoMessage(queueIDs.mid(0, maxSize));
            sendQueueEntryInfoMessage(queueIDs.mid(maxSize));
            return;
        }

        QByteArray message;
        message.reserve(4 + queueIDs.size() * (8 + NetworkProtocol::FILEHASH_BYTECOUNT));

        NetworkProtocol::append2Bytes(message,
                                      ServerMessageType::BulkQueueEntryHashMessage);
        NetworkUtil::append2Bytes(message, (uint)queueIDs.size());

        PlayerQueue& queue = _player->queue();

        /* TODO: bug: concurrency issue here when a QueueEntry has just been deleted */

        for (auto queueID : queueIDs)
        {
            auto track = queue.lookup(queueID);
            auto trackStatus =
                track ? createTrackStatusFor(track)
                      : NetworkProtocol::createTrackStatusUnknownId();

            auto hash = track ? track->hash() : null;

            NetworkUtil::append4Bytes(message, queueID);
            NetworkUtil::append2Bytes(message, trackStatus);
            NetworkUtil::append2Bytes(message, 0); /* filler */
            NetworkProtocol::appendHash(message, hash);
        }

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendPossibleTrackFilenames(quint32 queueID,
                                                     QVector<QString> const& names)
    {
        QByteArray message;
        message.reserve(2 + 4 + names.size() * (4 + 30)); /* only an approximation */
        NetworkProtocol::append2Bytes(message,
                                ServerMessageType::PossibleFilenamesForQueueEntryMessage);
        NetworkUtil::append4Bytes(message, queueID);

        for (auto& name : names)
        {
            QByteArray nameBytes = name.toUtf8();
            NetworkUtil::append4BytesSigned(message, nameBytes.size());
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
        NetworkProtocol::append2Bytes(message,
                                      ServerMessageType::NewUserAccountSaltMessage);
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
        NetworkProtocol::append2Bytes(message, ServerMessageType::UserLoginSaltMessage);
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

    void ConnectedClient::onCollectionTrackInfoBatchToSend(uint clientReference,
                                                      QVector<CollectionTrackInfo> tracks)
    {
        sendTrackInfoBatchMessage(clientReference, false, tracks);
    }

    void ConnectedClient::sendTrackAvailabilityBatchMessage(QVector<FileHash> available,
                                                            QVector<FileHash> unavailable)
    {
        if (_clientProtocolNo < 11) /* only send this if the client will understand */
            return;

        const int maxSize = (1 << 16) - 1;

        /* not too big? */
        if (available.size() > maxSize)
        {
            /* TODO: maybe delay the second part? */
            sendTrackAvailabilityBatchMessage(available.mid(0, maxSize), unavailable);
            sendTrackAvailabilityBatchMessage(available.mid(maxSize), unavailable);
            return;
        }
        if (unavailable.size() > maxSize)
        {
            /* TODO: maybe delay the second part? */
            sendTrackAvailabilityBatchMessage(available, unavailable.mid(0, maxSize));
            sendTrackAvailabilityBatchMessage(available, unavailable.mid(maxSize));
            return;
        }

        qDebug() << "sending track availability notification batch message;"
                 << "available count:" << available.size()
                 << "unavailable count:" << unavailable.size();

        QByteArray message;
        message.reserve(2 + 2 + 4
                        + NetworkProtocol::FILEHASH_BYTECOUNT * available.size()
                        + NetworkProtocol::FILEHASH_BYTECOUNT * unavailable.size());

        NetworkProtocol::append2Bytes(message,
                      ServerMessageType::CollectionAvailabilityChangeNotificationMessage);
        NetworkUtil::append2Bytes(message, 0) /* filler */;
        NetworkUtil::append2Bytes(message, available.size());
        NetworkUtil::append2Bytes(message, unavailable.size());

        for (int i = 0; i < available.size(); ++i)
        {
            NetworkProtocol::appendHash(message, available[i]);
        }

        for (int i = 0; i < unavailable.size(); ++i)
        {
            NetworkProtocol::appendHash(message, unavailable[i]);
        }

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendTrackInfoBatchMessage(uint clientReference,
                                                    bool isNotification,
                                                    QVector<CollectionTrackInfo> tracks)
    {
        const int maxSize = (1 << 16) - 1;

        /* not too big? */
        if (tracks.size() > maxSize)
        {
            /* TODO: maybe delay the second part? */
            sendTrackInfoBatchMessage(clientReference, isNotification,
                                      tracks.mid(0, maxSize));
            sendTrackInfoBatchMessage(clientReference, isNotification,
                                      tracks.mid(maxSize));
            return;
        }

        bool withAlbumAndTrackLength = _clientProtocolNo >= 7;

        /* estimate how much bytes we will need and reserve that memory in the buffer */
        const int bytesEstimatedPerTrack =
            NetworkProtocol::FILEHASH_BYTECOUNT + 1 + 2 + 2 + 20 + 15
                + (withAlbumAndTrackLength ? 2 + 15 + 4 : 0);
        QByteArray message;
        message.reserve(2 + 2 + 4 + tracks.size() * bytesEstimatedPerTrack);

        NetworkProtocol::append2Bytes(
            message,
            isNotification ? ServerMessageType::CollectionChangeNotificationMessage
                           : ServerMessageType::CollectionFetchResponseMessage
        );
        NetworkUtil::append2Bytes(message, tracks.size());
        if (!isNotification)
        {
            NetworkUtil::append4Bytes(message, clientReference);
        }

        for (int i = 0; i < tracks.size(); ++i)
        {
            const CollectionTrackInfo& track = tracks[i];

            QString title = track.title();
            QString artist = track.artist();
            QString album = track.album();

            /* worst case: 4 bytes in UTF-8 for each char */
            title.truncate(maxSize / 4);
            artist.truncate(maxSize / 4);
            album.truncate(maxSize / 4);

            QByteArray titleData = title.toUtf8();
            QByteArray artistData = artist.toUtf8();
            QByteArray albumData = album.toUtf8();

            NetworkProtocol::appendHash(message, track.hash());
            NetworkUtil::appendByte(message, track.isAvailable() ? 1 : 0);
            NetworkUtil::append2Bytes(message, (uint)titleData.size());
            NetworkUtil::append2Bytes(message, (uint)artistData.size());
            if (withAlbumAndTrackLength)
            {
                NetworkUtil::append2Bytes(message, (uint)albumData.size());
                NetworkUtil::append4BytesSigned(message, track.lengthInMilliseconds());
            }

            message += titleData;
            message += artistData;
            if (withAlbumAndTrackLength)
                message += albumData;
        }

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendNewHistoryEntryMessage(
                                                 QSharedPointer<PlayerHistoryEntry> entry)
    {
        QByteArray message;
        message.reserve(2 + 2 + 4 + 4 + 8 + 8 + 2 + 2);

        quint16 status = (entry->hadError() ? 1 : 0) | (entry->hadSeek() ? 2 : 0);

        NetworkProtocol::append2Bytes(message, ServerMessageType::NewHistoryEntryMessage);
        NetworkUtil::append2Bytes(message, 0); /* filler */
        NetworkUtil::append4Bytes(message, entry->queueID());
        NetworkUtil::append4Bytes(message, entry->user());
        NetworkUtil::append8ByteQDateTimeMsSinceEpoch(message, entry->started());
        NetworkUtil::append8ByteQDateTimeMsSinceEpoch(message, entry->ended());
        NetworkUtil::append2BytesSigned(message, entry->permillage());
        NetworkUtil::append2Bytes(message, status);

        sendBinaryMessage(message);
    }

    void ConnectedClient::onScrobblingProviderInfo(ScrobblingProvider provider,
                                                   ScrobblerStatus status, bool enabled)
    {
        auto userId = _serverInterface->userLoggedIn();
        sendScrobblingProviderInfoMessage(userId, provider, status, enabled);
    }

    void ConnectedClient::onScrobblerStatusChanged(ScrobblingProvider provider,
                                                   ScrobblerStatus status)
    {
        auto userId = _serverInterface->userLoggedIn();
        sendScrobblerStatusChangedMessage(userId, provider, status);
    }

    void ConnectedClient::onScrobblingProviderEnabledChanged(ScrobblingProvider provider,
                                                             bool enabled)
    {
        auto userId = _serverInterface->userLoggedIn();
        sendScrobblingProviderEnabledChangeMessage(userId, provider, enabled);
    }

    void ConnectedClient::onHashAvailabilityChanged(QVector<FileHash> available,
                                                    QVector<FileHash> unavailable)
    {
        sendTrackAvailabilityBatchMessage(available, unavailable);
    }

    void ConnectedClient::onHashInfoChanged(QVector<CollectionTrackInfo> changes)
    {
        sendTrackInfoBatchMessage(0, true, changes);
    }

    void ConnectedClient::onCollectionTrackInfoCompleted(uint clientReference)
    {
        sendSuccessMessage(clientReference, 0);
    }

    void ConnectedClient::sendServerNameMessage()
    {
        quint8 type = 0; // TODO : type
        auto name = _serverInterface->getServerCaption();

        name.truncate(63);
        QByteArray nameBytes = name.toUtf8();

        QByteArray message;
        message.reserve(2 + 2 + nameBytes.size());
        NetworkProtocol::append2Bytes(message, ServerMessageType::ServerNameMessage);
        NetworkUtil::appendByte(message, 0); /* unused */
        NetworkUtil::appendByte(message, type);
        message += nameBytes;

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendServerHealthMessageIfNotEverythingOkay()
    {
        if (!_serverHealthMonitor->anyProblem()) return;

        sendServerHealthMessage();
    }

    void ConnectedClient::sendServerHealthMessage()
    {
        /* only send it if the client will understand it */
        if (_clientProtocolNo < 10) return;

        quint16 problems = 0;
        problems |= _serverHealthMonitor->databaseUnavailable() ? 1u : 0u;
        problems |= _serverHealthMonitor->sslLibrariesMissing() ? 2u : 0u;

        qDebug() << "sending server health message with payload"
                 << QString::number(problems, 16) << "(hex)";

        QByteArray message;
        message.reserve(2 + 2);
        NetworkProtocol::append2Bytes(message, ServerMessageType::ServerHealthMessage);
        NetworkUtil::append2Bytes(message, problems);

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendServerClockMessage()
    {
        /* only send it if the client will understand it */
        if (_clientProtocolNo < 16)
            return;

        qint64 msSinceEpoch = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();

        QByteArray message;
        message.reserve(2 + 2 + 8);
        NetworkProtocol::append2Bytes(message, ServerMessageType::ServerClockMessage);
        NetworkUtil::append2Bytes(message, 0); /* filler */
        NetworkUtil::append8BytesSigned(message, msSinceEpoch);

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendScrobblingProviderInfoMessage(quint32 userId,
                                                            ScrobblingProvider provider,
                                                            ScrobblerStatus status,
                                                            bool enabled)
    {
        /* only send it if the client will understand it */
        if (_extensionsOther.isNotSupported(NetworkProtocolExtension::Scrobbling, 1))
            return;

        qDebug() << "sending scrobbling provider info message";

        QByteArray message;
        message.reserve(2 + 2 + 4 + 4);
        appendScrobblingMessageStart(message,
                                     ScrobblingServerMessageType::ProviderInfoMessage);
        NetworkUtil::append2Bytes(message, 0); /* filler */
        NetworkUtil::appendByte(message, NetworkProtocol::encode(provider));
        NetworkUtil::appendByte(message, NetworkProtocol::encode(status));
        NetworkUtil::appendByte(message, enabled ? 1 : 0);
        NetworkUtil::appendByte(message, 0); /* filler */
        NetworkUtil::append4Bytes(message, userId);

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendScrobblerStatusChangedMessage(quint32 userId,
                                                            ScrobblingProvider provider,
                                                            ScrobblerStatus newStatus)
    {
        /* only send it if the client will understand it */
        if (_extensionsOther.isNotSupported(NetworkProtocolExtension::Scrobbling, 1))
            return;

        QByteArray message;
        message.reserve(2 + 1 + 1 + 4);
        appendScrobblingMessageStart(message,
                                     ScrobblingServerMessageType::StatusChangeMessage);
        NetworkUtil::appendByte(message, NetworkProtocol::encode(provider));
        NetworkUtil::appendByte(message, NetworkProtocol::encode(newStatus));
        NetworkUtil::append4Bytes(message, userId);

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendScrobblingProviderEnabledChangeMessage(quint32 userId,
                                                              ScrobblingProvider provider,
                                                              bool enabled)
    {
        /* only send it if the client will understand it */
        if (_extensionsOther.isNotSupported(NetworkProtocolExtension::Scrobbling, 1))
            return;

        QByteArray message;
        message.reserve(2 + 1 + 1 + 4);
        appendScrobblingMessageStart(message,
                               ScrobblingServerMessageType::ProviderEnabledChangeMessage);
        NetworkUtil::appendByte(message, NetworkProtocol::encode(provider));
        NetworkUtil::appendByte(message, enabled ? 1 : 0);
        NetworkUtil::append4Bytes(message, userId);

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendDelayedStartInfoMessage()
    {
        /* only send it if the client will understand it */
        if (_clientProtocolNo < 21)
            return;

        qint64 msTimeRemaining =
                _serverInterface->getDelayedStartTimeRemainingMilliseconds();

        if (msTimeRemaining < 0)
        {
            qWarning() << "delayed start time remaining is negative, cannot send info";
            return;
        }

        qint64 msSinceEpoch = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();

        QByteArray message;
        message.reserve(2 + 2 + 8 + 8);
        NetworkProtocol::append2Bytes(message,
                                      ServerMessageType::DelayedStartInfoMessage);
        NetworkUtil::append2Bytes(message, 0); /* filler */
        NetworkUtil::append8BytesSigned(message, msSinceEpoch);
        NetworkUtil::append8BytesSigned(message, msTimeRemaining);

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendSuccessMessage(quint32 clientReference, quint32 intData)
    {
        sendResultMessage(ResultMessageErrorCode::NoError, clientReference, intData);
    }

    void ConnectedClient::sendSuccessMessage(quint32 clientReference, quint32 intData,
                                             QByteArray const& blobData)
    {
        sendResultMessage(ResultMessageErrorCode::NoError, clientReference, intData,
                          blobData);
    }

    void ConnectedClient::sendScrobblingResultMessage(ScrobblingResultMessageCode code,
                                                      quint32 clientReference)
    {
        auto extensionId =
            _extensionsThis.getExtensionId(NetworkProtocolExtension::Scrobbling);

        sendExtensionResultMessage(extensionId, quint8(code), clientReference);
    }

    void ConnectedClient::sendResultMessage(const Result& result, quint32 clientReference)
    {
        switch (result.code())
        {
        case ResultCode::Success:
            sendResultMessage(ResultMessageErrorCode::NoError, clientReference);
            return;
        case ResultCode::NoOp:
            sendResultMessage(ResultMessageErrorCode::AlreadyDone, clientReference);
            return;
        case ResultCode::NotLoggedIn:
            sendResultMessage(ResultMessageErrorCode::NotLoggedIn, clientReference);
            return;
        case ResultCode::OperationAlreadyRunning:
            sendResultMessage(ResultMessageErrorCode::OperationAlreadyRunning,
                              clientReference);
            return;
        case ResultCode::HashIsNull:
        case ResultCode::HashIsUnknown:
            sendResultMessage(ResultMessageErrorCode::InvalidHash, clientReference);
            return;
        case ResultCode::QueueEntryIdNotFound:
            sendResultMessage(ResultMessageErrorCode::QueueIdNotFound, clientReference,
                              static_cast<quint32>(result.intArg()));
            return;
        case ResultCode::QueueIndexOutOfRange:
            sendResultMessage(ResultMessageErrorCode::InvalidQueueIndex, clientReference);
            return;
        case ResultCode::QueueMaxSizeExceeded:
            sendResultMessage(ResultMessageErrorCode::MaximumQueueSizeExceeded,
                              clientReference);
            return;
        case ResultCode::QueueItemTypeInvalid:
            sendResultMessage(ResultMessageErrorCode::InvalidQueueItemType,
                              clientReference);
            return;
        case ResultCode::DelayOutOfRange:
            sendResultMessage(ResultMessageErrorCode::InvalidTimeSpan, clientReference);
            return;
        case ResultCode::ScrobblingSystemDisabled:
            sendScrobblingResultMessage(
                ScrobblingResultMessageCode::ScrobblingSystemDisabled, clientReference
            );
            return;
        case ResultCode::ScrobblingProviderInvalid:
            sendScrobblingResultMessage(
                ScrobblingResultMessageCode::ScrobblingProviderInvalid, clientReference
            );
            return;
        case ResultCode::ScrobblingProviderNotEnabled:
            sendScrobblingResultMessage(
                ScrobblingResultMessageCode::ScrobblingProviderNotEnabled, clientReference
            );
            return;
        case ResultCode::ScrobblingAuthenticationFailed:
            sendScrobblingResultMessage(
                ScrobblingResultMessageCode::ScrobblingAuthenticationFailed,
                clientReference
            );
            return;
        case ResultCode::UnspecifiedScrobblingBackendError:
            sendScrobblingResultMessage(
                ScrobblingResultMessageCode::UnspecifiedScrobblingBackendError,
                clientReference
            );
            return;
        case ResultCode::NotImplementedError:
        case ResultCode::InternalError:
            sendResultMessage(ResultMessageErrorCode::NonFatalInternalServerError,
                              clientReference);
            return;
        }

        qWarning() << "Unhandled ResultCode" << int(result.code())
                   << "for client-ref" << clientReference;
        sendResultMessage(ResultMessageErrorCode::UnknownError, clientReference);
    }

    void ConnectedClient::sendResultMessage(ResultMessageErrorCode errorType,
                                            quint32 clientReference)
    {
        return sendResultMessage(errorType, clientReference, 0);
    }

    void ConnectedClient::sendResultMessage(ResultMessageErrorCode errorType,
                                            quint32 clientReference, quint32 intData)
    {
        QByteArray data; /* empty data */
        sendResultMessage(errorType, clientReference, intData, data);
    }

    void ConnectedClient::sendResultMessage(ResultMessageErrorCode errorType,
                                            quint32 clientReference, quint32 intData,
                                            QByteArray const& blobData)
    {
        QByteArray message;
        message.reserve(2 + 2 + 4 + 4 + blobData.size());
        NetworkProtocol::append2Bytes(message, ServerMessageType::SimpleResultMessage);
        NetworkProtocol::append2Bytes(message, errorType);
        NetworkUtil::append4Bytes(message, clientReference);
        NetworkUtil::append4Bytes(message, intData);

        message += blobData;

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendExtensionResultMessage(quint8 extensionId,
                                                     quint8 resultCode,
                                                     quint32 clientReference)
    {
        if (_clientProtocolNo < 23 /* client does not support this message */
            || extensionId == 0 /* OR extension not supported by the client */)
        {
            sendExtensionResultMessageAsRegularResultMessage(extensionId, resultCode,
                                                             clientReference);
            return;
        }

        auto message =
            NetworkProtocolExtensionMessages::generateExtensionResultMessage(
                extensionId, resultCode, clientReference
            );

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendExtensionResultMessageAsRegularResultMessage(
                                                                quint8 extensionId,
                                                                quint8 resultCode,
                                                                quint32 clientReference)
    {
        Q_UNUSED(extensionId);

        /* Send a regular result message with a generic (error) code.
           We assume that the extension-specific code zero indicates success. */

        if (resultCode == 0)
            sendResultMessage(ResultMessageErrorCode::NoError, clientReference);
        else
            sendResultMessage(ResultMessageErrorCode::UnknownError, clientReference);
    }

    void ConnectedClient::sendNonFatalInternalErrorResultMessage(quint32 clientReference)
    {
        sendResultMessage(ResultMessageErrorCode::NonFatalInternalServerError,
                          clientReference, 0);
    }

    void ConnectedClient::serverHealthChanged(bool databaseUnavailable,
                                              bool sslLibrariesMissing)
    {
        Q_UNUSED(databaseUnavailable)
        Q_UNUSED(sslLibrariesMissing)

        sendServerHealthMessage();
    }

    void ConnectedClient::serverSettingsReloadResultEvent(uint clientReference,
                                                         ResultMessageErrorCode errorCode)
    {
        sendResultMessage(errorCode, clientReference);
    }

    void ConnectedClient::volumeChanged(int volume)
    {
        Q_UNUSED(volume)

        sendVolumeMessage();
    }

    void ConnectedClient::onDynamicModeStatusEvent(StartStopEventStatus dynamicModeStatus,
                                                   int noRepetitionSpanSeconds)
    {
        sendDynamicModeStatusMessage(dynamicModeStatus, noRepetitionSpanSeconds);
    }

    void ConnectedClient::onDynamicModeWaveStatusEvent(StartStopEventStatus waveStatus,
                                                       quint32 user,
                                                       int waveDeliveredCount,
                                                       int waveTotalCount)
    {
        sendGeneratorWaveStatusMessage(waveStatus, user,
                                       waveDeliveredCount, waveTotalCount);
    }

    void ConnectedClient::onUserPlayingForChanged(quint32 user)
    {
        Q_UNUSED(user)

        sendUserPlayingForModeMessage();
    }

    void ConnectedClient::onFullIndexationRunStatusChanged(bool running)
    {
        auto event =
            running
                ? ServerEventCode::FullIndexationRunning
                : ServerEventCode::FullIndexationNotRunning;

        sendEventNotificationMessage(event);
    }

    void ConnectedClient::playerStateChanged(ServerPlayerState state)
    {
        if (_binaryMode)
        {
            sendPlayerStateMessage();
            return;
        }

        switch (state)
        {
        case ServerPlayerState::Playing:
            sendTextCommand("playing");
            break;
        case ServerPlayerState::Paused:
            sendTextCommand("paused");
            break;
        case ServerPlayerState::Stopped:
            sendTextCommand("stopped");
            break;
        }
    }

    void ConnectedClient::currentTrackChanged(QSharedPointer<QueueEntry const> entry)
    {
        if (_binaryMode)
        {
            sendPlayerStateMessage();
            return;
        }

        if (entry == nullptr)
        {
            sendTextCommand("nowplaying nothing");
            return;
        }

        int seconds = static_cast<int>(entry->lengthInMilliseconds() / 1000);
        auto hash = entry->hash().value();

        sendTextCommand(
            "nowplaying track\n QID: " + QString::number(entry->queueID())
             + "\n position: " + QString::number(_player->playPosition())
             + "\n title: " + entry->title()
             + "\n artist: " + entry->artist()
             + "\n length: " + (seconds < 0 ? "?" : QString::number(seconds)) + " sec"
             + "\n hash length: " + QString::number(hash.length())
             + "\n hash SHA-1: " + hash.SHA1().toHex()
             + "\n hash MD5: " + hash.MD5().toHex()
        );
    }

    void ConnectedClient::newHistoryEntry(QSharedPointer<PlayerHistoryEntry> entry)
    {
        if (_binaryMode)
            sendNewHistoryEntryMessage(entry);
    }

    void ConnectedClient::trackPositionChanged(qint64 position)
    {
        Q_UNUSED(position)

        if (_binaryMode)
        {
            sendPlayerStateMessage();
            return;
        }

        //sendTextCommand("position " + QString::number(position));
    }

    void ConnectedClient::onDelayedStartActiveChanged()
    {
        if (_clientProtocolNo >= 21 && _serverInterface->delayedStartActive())
            sendDelayedStartInfoMessage();
        else
            sendPlayerStateMessage();
    }

    void ConnectedClient::sendTextualQueueInfo()
    {
        PlayerQueue& queue = _player->queue();
        const auto queueContent = queue.entries(0, 10);

        QString command =
            "queue length " + QString::number(queue.length())
            + "\nIndex|  QID  | Length | Title                          | Artist";

        command.reserve(command.size() + 100 * queueContent.size());

        Resolver& resolver = _player->resolver();
        uint index = 0;
        for (auto& entry : queueContent)
        {
            ++index;
            entry->checkTrackData(resolver);

            int lengthInSeconds =
                static_cast<int>(entry->lengthInMilliseconds() / 1000);

            command += "\n";
            command += QString::number(index).rightJustified(5);
            command += "|";
            command += QString::number(entry->queueID()).rightJustified(7);
            command += "|";

            if (lengthInSeconds < 0)
            {
                command += "        |";
            }
            else
            {
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

    void ConnectedClient::schedulePlayerStateNotification()
    {
        if (_pendingPlayerStatus) return;

        _pendingPlayerStatus = true;
        QTimer::singleShot(25, this, &ConnectedClient::sendStateInfoAfterTimeout);
    }

    void ConnectedClient::queueEntryRemoved(qint32 offset, quint32 queueID)
    {
        sendQueueEntryRemovedMessage(offset, queueID);
        schedulePlayerStateNotification(); /* queue length changed, notify after delay */
    }

    void ConnectedClient::queueEntryAddedWithoutReference(qint32 index, quint32 queueId)
    {
        sendQueueEntryAddedMessage(index, queueId);

        schedulePlayerStateNotification(); /* queue length changed, notify after delay */
    }

    void ConnectedClient::queueEntryAddedWithReference(qint32 index, quint32 queueId,
                                                       quint32 clientReference)
    {
        if (_clientProtocolNo >= 9)
        {
            sendQueueEntryAdditionConfirmationMessage(clientReference, index, queueId);
        }
        else
        {
            sendSuccessMessage(clientReference, queueId);
        }

        schedulePlayerStateNotification(); /* queue length changed, notify after delay */
    }

    void ConnectedClient::queueEntryMoved(quint32 fromOffset, quint32 toOffset,
                                          quint32 queueID)
    {
        sendQueueEntryMovedMessage(fromOffset, toOffset, queueID);
    }

    void ConnectedClient::readBinaryCommands()
    {
        char lengthBytes[4];

        while
            (_socket->isOpen()
            && _socket->peek(lengthBytes, sizeof(lengthBytes)) == sizeof(lengthBytes))
        {
            quint32 messageLength = NetworkUtil::get4Bytes(lengthBytes);

            if (_socket->bytesAvailable() - sizeof(lengthBytes) < messageLength)
            {
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

    void ConnectedClient::handleBinaryMessage(QByteArray const& message)
    {
        if (message.length() < 2)
        {
            qDebug() << "received invalid binary message (less than 2 bytes)";
            return; /* invalid message */
        }

        quint16 messageType = NetworkUtil::get2Bytes(message, 0);
        if (messageType & (1u << 15))
        {
            quint8 extensionMessageType = messageType & 0x7Fu;
            quint8 extensionId = (messageType >> 7) & 0xFFu;

            handleExtensionMessage(extensionId, extensionMessageType, message);
        }
        else
        {
            auto clientMessageType = static_cast<ClientMessageType>(messageType);

            handleStandardBinaryMessage(clientMessageType, message);
        }
    }

    void ConnectedClient::handleStandardBinaryMessage(ClientMessageType messageType,
                                                      QByteArray const& message)
    {
        qint32 messageLength = message.length();

        switch (messageType)
        {
        case ClientMessageType::KeepAliveMessage:
            parseKeepAliveMessage(message);
            break;
        case ClientMessageType::ClientExtensionsMessage:
            parseClientProtocolExtensionsMessage(message);
            break;
        case ClientMessageType::SingleByteActionMessage:
            parseSingleByteActionMessage(message);
            break;
        case ClientMessageType::ParameterlessActionMessage:
            parseParameterlessActionMessage(message);
            break;
        case ClientMessageType::TrackInfoRequestMessage:
            parseTrackInfoRequestMessage(message);
            break;
        case ClientMessageType::BulkTrackInfoRequestMessage:
            parseBulkTrackInfoRequestMessage(message);
            break;
        case ClientMessageType::BulkQueueEntryHashRequestMessage:
            parseBulkQueueEntryHashRequestMessage(message);
            break;
        case ClientMessageType::QueueFetchRequestMessage:
            parseQueueFetchRequestMessage(message);
            break;
        case ClientMessageType::QueueEntryRemovalRequestMessage:
            parseQueueEntryRemovalRequest(message);
            break;
        case ClientMessageType::GeneratorNonRepetitionChangeMessage:
            parseGeneratorNonRepetitionChangeMessage(message);
            break;
        case ClientMessageType::PossibleFilenamesForQueueEntryRequestMessage:
            parsePossibleFilenamesForQueueEntryRequestMessage(message);
            break;
        case ClientMessageType::ActivateDelayedStartRequest:
            parseActivateDelayedStartRequest(message);
            break;
        case ClientMessageType::PlayerSeekRequestMessage:
            parsePlayerSeekRequestMessage(message);
            break;
        case ClientMessageType::QueueEntryMoveRequestMessage:
            parseQueueEntryMoveRequestMessage(message);
            break;
        case ClientMessageType::InitiateNewUserAccountMessage:
            parseInitiateNewUserAccountMessage(message);
            break;
        case ClientMessageType::FinishNewUserAccountMessage:
            parseFinishNewUserAccountMessage(message);
            break;
        case ClientMessageType::InitiateLoginMessage:
            parseInitiateLoginMessage(message);
            break;
        case ClientMessageType::FinishLoginMessage:
            parseFinishLoginMessage(message);
            break;
        case ClientMessageType::CollectionFetchRequestMessage:
            parseCollectionFetchRequestMessage(message);
            break;
        case ClientMessageType::AddHashToEndOfQueueRequestMessage:
        case ClientMessageType::AddHashToFrontOfQueueRequestMessage:
            parseAddHashToQueueRequest(message, messageType);
            break;
        case ClientMessageType::HashUserDataRequestMessage:
            parseHashUserDataRequest(message);
            break;
        case ClientMessageType::InsertSpecialQueueItemRequest:
            parseInsertSpecialQueueItemRequest(message);
            break;
        case ClientMessageType::InsertHashIntoQueueRequestMessage:
            parseInsertHashIntoQueueRequest(message);
            break;
        case ClientMessageType::PlayerHistoryRequestMessage:
            parsePlayerHistoryRequest(message);
            break;
        case ClientMessageType::QueueEntryDuplicationRequestMessage:
            parseQueueEntryDuplicationRequest(message);
            break;
        default:
            qDebug() << "received unknown binary message type"
                     << static_cast<int>(messageType)
                     << "with length" << messageLength;
            break; /* unknown message type */
        }
    }

    void ConnectedClient::handleExtensionMessage(quint8 extensionId, quint8 messageType,
                                                 QByteArray const& message)
    {
        /* parse extensions here */

        auto extension = _extensionsOther.getExtensionById(extensionId);

        //if (extension == NetworkProtocolExtension::ExtensionName1)
        //{
        //    switch (messageType)
        //    {
        //    case 1: parseExtensionMessage1(message); return;
        //    case 2: parseExtensionMessage2(message); return;
        //    case 3: parseExtensionMessage3(message); return;
        //    }
        //}

        if (extension == NetworkProtocolExtension::Scrobbling)
        {
            switch (static_cast<ScrobblingClientMessageType>(messageType))
            {
            case ScrobblingClientMessageType::ProviderInfoRequestMessage:
                parseCurrentUserScrobblingProviderInfoRequestMessage(message);
                return;
            case ScrobblingClientMessageType::EnableDisableRequestMessage:
                parseUserScrobblingEnableDisableRequest(message);
                return;
            case ScrobblingClientMessageType::AuthenticationRequestMessage:
                parseScrobblingAuthenticationRequestMessage(message);
                return;
            }
        }

        qWarning() << "unhandled extension message" << int(messageType)
                   << "for extension" << int(extensionId)
                   << "with length" << message.length()
                   << "; extension: " << toString(extension);
    }

    void ConnectedClient::parseKeepAliveMessage(const QByteArray& message)
    {
        if (message.length() != 4)
            return; /* invalid message */

        quint16 blob = NetworkUtil::get2Bytes(message, 2);

        sendKeepAliveReply(blob);
    }

    void ConnectedClient::parseClientProtocolExtensionsMessage(QByteArray const& message)
    {
        auto supportMapOrNull =
            NetworkProtocolExtensionMessages::parseExtensionSupportMessage(message);

        if (supportMapOrNull.hasValue())
            _extensionsOther = supportMapOrNull.value();
    }

    void ConnectedClient::parseSingleByteActionMessage(const QByteArray& message)
    {
        if (message.length() != 3)
            return; /* invalid message */

        quint8 actionType = NetworkUtil::getByte(message, 2);
        handleSingleByteAction(actionType);
    }

    void ConnectedClient::parseParameterlessActionMessage(const QByteArray& message)
    {
        if (message.length() != 8)
            return; /* invalid message */

        int numericActionCode = NetworkUtil::get2BytesUnsignedToInt(message, 2);
        quint32 clientReference = NetworkUtil::get4Bytes(message, 4);

        qDebug() << "received parameterless action" << numericActionCode
                 << "with client-ref" << clientReference;

        auto action = static_cast<ParameterlessActionCode>(numericActionCode);

        handleParameterlessAction(action, clientReference);
    }

    void ConnectedClient::parseInitiateNewUserAccountMessage(const QByteArray& message)
    {
        if (message.length() <= 8)
            return; /* invalid message */

        int loginLength = NetworkUtil::getByteUnsignedToInt(message, 2);
        quint32 clientReference = NetworkUtil::get4Bytes(message, 4);

        if (message.length() - 8 != loginLength)
            return; /* invalid message */

        qDebug() << "received initiate-new-user-account request; clientRef:"
                 << clientReference;

        QByteArray loginBytes = message.mid(8);
        QString login = QString::fromUtf8(loginBytes);

        auto result = Users::generateSaltForNewAccount(login);

        if (result.succeeded())
        {
            auto salt = result.result();
            _userAccountRegistering = login;
            _saltForUserAccountRegistering = salt;
            sendNewUserAccountSaltMessage(login, salt);
        }
        else
        {
            sendResultMessage(
                Users::toNetworkProtocolError(result.error()), clientReference, 0,
                loginBytes
            );
        }
    }

    void ConnectedClient::parseFinishNewUserAccountMessage(const QByteArray& message)
    {
        if (message.length() <= 8)
            return; /* invalid message */

        int loginLength = NetworkUtil::getByteUnsignedToInt(message, 2);
        int saltLength = NetworkUtil::getByteUnsignedToInt(message, 3);
        quint32 clientReference = NetworkUtil::get4Bytes(message, 4);

        qDebug() << "received finish-new-user-account request; clientRef:"
                 << clientReference;

        if (message.length() - 8 <= loginLength + saltLength)
        {
            sendResultMessage(
                ResultMessageErrorCode::InvalidMessageStructure, clientReference
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
                ResultMessageErrorCode::UserAccountRegistrationMismatch,
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

        if (hashedPasswordFromClient.size() != hashTest.size())
        {
            sendResultMessage(
                ResultMessageErrorCode::InvalidMessageStructure, clientReference
            );
            return;
        }

        auto result =
            _users->registerNewAccount(
                login, saltFromClient, hashedPasswordFromClient
            );

        if (result.succeeded())
        {
            sendSuccessMessage(clientReference, result.result());
        }
        else
        {
            sendResultMessage(Users::toNetworkProtocolError(result.error()),
                              clientReference);
        }
    }

    void ConnectedClient::parseInitiateLoginMessage(const QByteArray& message)
    {
        if (message.length() <= 8)
            return; /* invalid message */

        int loginLength = NetworkUtil::getByteUnsignedToInt(message, 2);
        quint32 clientReference = NetworkUtil::get4Bytes(message, 4);

        if (message.length() - 8 != loginLength)
            return; /* invalid message */

        QByteArray loginBytes = message.mid(8);
        QString login = QString::fromUtf8(loginBytes);

        qDebug() << "received initiate-login request; clientRef:" << clientReference
                 << "; login:" << login;

        if (isLoggedIn()) /* already logged in */
        {
            sendResultMessage(ResultMessageErrorCode::AlreadyLoggedIn,
                              clientReference);
            return;
        }

        DatabaseRecords::User user;
        bool userLookup = _users->getUserByLogin(login, user);

        if (!userLookup) /* user does not exist */
        {
            sendResultMessage(
                ResultMessageErrorCode::UserLoginAuthenticationFailed,
                clientReference, 0, loginBytes
            );
            return;
        }

        QByteArray userSalt = user.salt;
        QByteArray sessionSalt = Users::generateSalt();

        _userIdLoggingIn = user.id;
        _sessionSaltForUserLoggingIn = sessionSalt;

        sendUserLoginSaltMessage(login, userSalt, sessionSalt);
    }

    void ConnectedClient::parseFinishLoginMessage(const QByteArray& message)
    {
        if (message.length() <= 12)
            return; /* invalid message */

        int loginLength = NetworkUtil::getByteUnsignedToInt(message, 4);
        int userSaltLength = NetworkUtil::getByteUnsignedToInt(message, 5);
        int sessionSaltLength = NetworkUtil::getByteUnsignedToInt(message, 6);
        int hashedPasswordLength = NetworkUtil::getByteUnsignedToInt(message, 7);
        quint32 clientReference = NetworkUtil::get4Bytes(message, 8);

        if (message.length() - 12 !=
                loginLength + userSaltLength + sessionSaltLength + hashedPasswordLength)
        {
            sendResultMessage(
                ResultMessageErrorCode::InvalidMessageStructure, clientReference
            );
            return; /* invalid message */
        }

        int offset = 12;

        QByteArray loginBytes = message.mid(offset, loginLength);
        QString login = QString::fromUtf8(loginBytes);
        offset += loginLength;

        QByteArray userSaltFromClient = message.mid(offset, userSaltLength);
        offset += userSaltLength;

        QByteArray sessionSaltFromClient = message.mid(offset, sessionSaltLength);
        offset += sessionSaltLength;

        QByteArray hashedPasswordFromClient = message.mid(offset);

        qDebug() << "received finish-login request; clientRef:" << clientReference
                 << "; login:" << login;

        DatabaseRecords::User user;
        bool userLookup = _users->getUserByLogin(login, user);

        if (!userLookup) /* user does not exist */
        {
            sendResultMessage(
                ResultMessageErrorCode::UserLoginAuthenticationFailed,
                clientReference, 0, loginBytes
            );
            return;
        }

        qDebug() << " user lookup successfull: user id =" << user.id
                 << "; login =" << user.login;

        if (user.id != _userIdLoggingIn
            || userSaltFromClient != user.salt
            || sessionSaltFromClient != _sessionSaltForUserLoggingIn)
        {
            sendResultMessage(
                ResultMessageErrorCode::UserAccountLoginMismatch,
                clientReference, 0, loginBytes
            );
            return;
        }

        /* clean up state */
        _userIdLoggingIn = 0;
        _sessionSaltForUserLoggingIn.clear();

        bool loginSucceeded =
            Users::checkUserLoginPassword(
                user, sessionSaltFromClient, hashedPasswordFromClient
            );

        if (loginSucceeded)
        {
            _serverInterface->setLoggedIn(user.id, user.login);
            connectSlotsAfterSuccessfulUserLogin(user.id);
            sendSuccessMessage(clientReference, user.id);
        }
        else
        {
            sendResultMessage(ResultMessageErrorCode::UserLoginAuthenticationFailed,
                              clientReference);
        }
    }

    void ConnectedClient::parseActivateDelayedStartRequest(const QByteArray& message)
    {
        if (message.length() != 16)
            return; /* invalid message */

        quint32 clientReference = NetworkUtil::get4Bytes(message, 4);
        qint64 delayMilliseconds = NetworkUtil::get8BytesSigned(message, 8);

        qDebug() << "received delayed start activation request; delay:"
                 << delayMilliseconds << "ms; client-ref:" << clientReference;

        auto result = _serverInterface->activateDelayedStart(delayMilliseconds);
        sendResultMessage(result, clientReference);
    }

    void ConnectedClient::parsePlayerSeekRequestMessage(const QByteArray& message)
    {
        if (message.length() != 14)
            return; /* invalid message */

        quint32 queueID = NetworkUtil::get4Bytes(message, 2);
        qint64 position = NetworkUtil::get8BytesSigned(message, 6);

        qDebug() << "received seek command; QID:" << queueID
                 << "  position:" << position;

        if (queueID != _player->nowPlayingQID())
            return; /* invalid queue ID */

        if (position < 0) return; /* invalid position */

        _serverInterface->seekTo(position);
    }

    void ConnectedClient::parseTrackInfoRequestMessage(const QByteArray& message)
    {
        if (message.length() != 6)
            return; /* invalid message */

        quint32 queueID = NetworkUtil::get4Bytes(message, 2);

        if (queueID <= 0)
            return; /* invalid queue ID */

        qDebug() << "received track info request for Q-ID" << queueID;

        sendQueueEntryInfoMessage(queueID);
    }

    void ConnectedClient::parseBulkTrackInfoRequestMessage(const QByteArray& message)
    {
        if (message.length() < 6 || (message.length() - 2) % 4 != 0)
            return; /* invalid message */

        QList<quint32> QIDs;
        QIDs.reserve((message.length() - 2) / 4);

        int offset = 2;
        while (offset <= message.length() - 4)
        {
            quint32 queueID = NetworkUtil::get4Bytes(message, offset);

            if (queueID > 0)
                QIDs.append(queueID);

            offset += 4;
        }

        qDebug() << "received bulk track info request for" << QIDs.size() << "QIDs";

        sendQueueEntryInfoMessage(QIDs);
    }

    void ConnectedClient::parseBulkQueueEntryHashRequestMessage(const QByteArray& message)
    {
        if (message.length() < 8 || (message.length() - 4) % 4 != 0)
            return; /* invalid message */

        QList<quint32> QIDs;
        QIDs.reserve((message.length() - 4) / 4);

        int offset = 4;
        while (offset <= message.length() - 4)
        {
            quint32 queueID = NetworkUtil::get4Bytes(message, offset);

            if (queueID > 0)
                QIDs.append(queueID);

            offset += 4;
        }

        qDebug() << "received bulk hash info request for" << QIDs.size() << "QIDs";

        sendQueueEntryHashMessage(QIDs);
    }

    void ConnectedClient::parsePossibleFilenamesForQueueEntryRequestMessage(
                                                                const QByteArray& message)
    {
        if (message.length() != 6)
            return; /* invalid message */

        quint32 queueId = NetworkUtil::get4Bytes(message, 2);
        qDebug() << "received request for possible filenames of QID" << queueId;

        auto future = _serverInterface->getPossibleFilenamesForQueueEntry(queueId);

        future.addResultListener(
            this,
            [this, queueId](QVector<QString> result)
            {
                sendPossibleTrackFilenames(queueId, result);
            }
        );
    }

    void ConnectedClient::parseQueueFetchRequestMessage(const QByteArray& message)
    {
        if (message.length() != 7)
            return; /* invalid message */

        if (!isLoggedIn())
            return; /* client needs to be authenticated for this */

        qint32 startOffset = NetworkUtil::get4BytesSigned(message, 2);
        quint8 length = NetworkUtil::getByte(message, 6);

        if (startOffset < 0)
            return; /* invalid message */

        qDebug() << "received queue fetch request; offset:" << startOffset
                 << "  length:" << length;

        sendQueueContentMessage(startOffset, length);
    }

    void ConnectedClient::parseAddHashToQueueRequest(const QByteArray& message,
                                                     ClientMessageType messageType)
    {
        qDebug() << "received 'add filehash to queue' request";

        if (message.length() != 2 + 2 + NetworkProtocol::FILEHASH_BYTECOUNT)
            return; /* invalid message */

        bool ok;
        FileHash hash = NetworkProtocol::getHash(message, 4, &ok);
        if (!ok || hash.isNull())
            return; /* invalid message */

        qDebug() << " request contains hash:" << hash.dumpToString();

        if (messageType == ClientMessageType::AddHashToEndOfQueueRequestMessage)
        {
            _serverInterface->insertTrackAtEnd(hash);
        }
        else if (messageType == ClientMessageType::AddHashToFrontOfQueueRequestMessage)
        {
            _serverInterface->insertTrackAtFront(hash);
        }
        else
        {
            return; /* invalid message */
        }
    }

    void ConnectedClient::parseInsertSpecialQueueItemRequest(QByteArray const& message)
    {
        qDebug() << "received 'insert special queue item' request";

        if (message.length() != 12)
            return; /* invalid message */

        quint8 itemTypeNumber = NetworkUtil::getByte(message, 2);
        quint8 indexTypeNumber = NetworkUtil::getByte(message, 3);
        quint32 clientReference = NetworkUtil::get4Bytes(message, 4);
        qint32 offset = NetworkUtil::get4BytesSigned(message, 8);

        SpecialQueueItemType itemType;
        if (itemTypeNumber == 1)
            itemType = SpecialQueueItemType::Break;
        else if (itemTypeNumber == 2)
            itemType = SpecialQueueItemType::Barrier;
        else
            return; /* invalid message */

        QueueIndexType indexType;
        if (indexTypeNumber == 0)
            indexType = QueueIndexType::Normal;
        else if (indexTypeNumber == 1)
            indexType = QueueIndexType::Reverse;
        else
            return; /* invalid message */

        auto result = _serverInterface->insertSpecialQueueItem(itemType, indexType,
                                                               offset, clientReference);

        /* success is handled by the queue insertion event, failure is handled here */
        if (!result)
            sendResultMessage(result, clientReference);
    }

    void ConnectedClient::parseInsertHashIntoQueueRequest(const QByteArray &message)
    {
        qDebug() << "received 'insert filehash into queue at index' request";

        if (message.length() != 2 + 2 + 4 + 4 + NetworkProtocol::FILEHASH_BYTECOUNT)
            return; /* invalid message */

        quint32 clientReference = NetworkUtil::get4Bytes(message, 4);
        qint32 index = NetworkUtil::get4BytesSigned(message, 8);

        qDebug() << " client ref:" << clientReference << "; " << "index:" << index;

        bool ok;
        FileHash hash = NetworkProtocol::getHash(message, 12, &ok);
        if (!ok || hash.isNull())
            return; /* invalid message */

        qDebug() << " request contains hash:" << hash.dumpToString();

        auto result = _serverInterface->insertTrack(hash, index, clientReference);

        /* success is handled by the queue insertion event, failure is handled here */
        if (!result)
            sendResultMessage(result, clientReference);
    }

    void ConnectedClient::parseQueueEntryRemovalRequest(QByteArray const& message)
    {
        if (message.length() != 6)
            return; /* invalid message */

        if (!isLoggedIn()) return; /* client needs to be authenticated for this */

        quint32 queueID = NetworkUtil::get4Bytes(message, 2);
        qDebug() << "received removal request for QID" << queueID;

        if (queueID <= 0)
            return; /* invalid queue ID */

        _serverInterface->removeQueueEntry(queueID);
    }

    void ConnectedClient::parseQueueEntryDuplicationRequest(QByteArray const& message)
    {
        qDebug() << "received 'duplicate queue entry' request";

        if (message.length() != 2 + 2 + 4 + 4)
            return; /* invalid message */

        quint32 clientReference = NetworkUtil::get4Bytes(message, 4);
        quint32 queueId = NetworkUtil::get4Bytes(message, 8);

        qDebug() << " client ref:" << clientReference << "; " << "QID:" << queueId;

        auto result = _serverInterface->duplicateQueueEntry(queueId, clientReference);

        /* success is handled by the queue insertion event, failure is handled here */
        if (!result)
            sendResultMessage(result, clientReference);
    }

    void ConnectedClient::parseQueueEntryMoveRequestMessage(const QByteArray& message)
    {
        if (message.length() != 8)
            return; /* invalid message */

        if (!isLoggedIn()) return; /* client needs to be authenticated for this */

        qint16 move = NetworkUtil::get2BytesSigned(message, 2);
        quint32 queueID = NetworkUtil::get4Bytes(message, 4);

        qDebug() << "received track move command; QID:" << queueID
                 << " move:" << move;

        _serverInterface->moveQueueEntry(queueID, move);
    }

    void ConnectedClient::parseHashUserDataRequest(const QByteArray& message)
    {
        if (message.length() <= 2 + 2 + 4)
            return; /* invalid message */

        if (!isLoggedIn()) { return; /* client needs to be authenticated for this */ }

        quint16 fields = NetworkUtil::get2Bytes(message, 2);
        quint32 userId = NetworkUtil::get4Bytes(message, 4);

        int offset = 2 + 2 + 4;
        int hashCount = (message.length() - offset) / NetworkProtocol::FILEHASH_BYTECOUNT;
        if ((message.length() - offset) % NetworkProtocol::FILEHASH_BYTECOUNT != 0)
            return; /* invalid message */

        qDebug() << "received request for user track data; user:" << userId
                 << " track count:" << hashCount << " fields:" << fields;

        fields = fields & 3; /* filter non-supported fields */

        if (fields == 0) return; /* no data that we can return */

        QVector<FileHash> hashes;
        hashes.reserve(hashCount);

        for (int i = 0; i < hashCount; ++i)
        {
            bool ok;
            auto hash = NetworkProtocol::getHash(message, offset, &ok);
            if (!ok || hash.isNull()) continue;

            hashes.append(hash);
            offset += NetworkProtocol::FILEHASH_BYTECOUNT;
        }

        _serverInterface->requestHashUserData(userId, hashes);
    }

    void ConnectedClient::parsePlayerHistoryRequest(const QByteArray& message)
    {
        qDebug() << "received player history list request";

        if (message.length() != 2 + 2)
            return; /* invalid message */

        if (!isLoggedIn()) { return; /* client needs to be authenticated for this */ }

        int limit = NetworkUtil::getByteUnsignedToInt(message, 3);

        sendQueueHistoryMessage(limit);
    }

    void ConnectedClient::parseCurrentUserScrobblingProviderInfoRequestMessage(
                                                                QByteArray const& message)
    {
        qDebug() << "received request for info about the user's scrobbling providers";

        if (message.length() != 2 + 2)
            return; /* invalid message */

        auto filler = NetworkUtil::get2Bytes(message, 2);
        if (filler != 0)
            return; /* invalid message */

        _serverInterface->requestScrobblingInfo();
    }

    void ConnectedClient::parseUserScrobblingEnableDisableRequest(
                                                                QByteArray const& message)
    {
        qDebug() << "received user scrobbling enable/disable request";

        if (message.length() != 2 + 2)
            return; /* invalid message */

        auto provider =
            NetworkProtocol::decodeScrobblingProvider(NetworkUtil::getByte(message, 2));

        auto enabled = NetworkUtil::getByte(message, 3) != 0;

        _serverInterface->setScrobblingProviderEnabled(provider, enabled);
    }

    void ConnectedClient::parseScrobblingAuthenticationRequestMessage(
                                                                const QByteArray& message)
    {
        qDebug() << "received scrobbling authentication request";

        if (message.length() < 12)
            return; /* invalid message */

        auto provider =
            NetworkProtocol::decodeScrobblingProvider(NetworkUtil::getByte(message, 2));

        quint8 keyId = NetworkUtil::getByte(message, 3);
        quint32 clientReference = NetworkUtil::get4Bytes(message, 4);
        qint32 obfuscatedByteCount = NetworkUtil::get4BytesSigned(message, 8);

        if (obfuscatedByteCount <= 0 || message.size() != 12 + obfuscatedByteCount)
        {
            return; /* invalid message */
        }

        auto obfuscatedBytes = message.mid(12);

        ObfuscatedScrobblingUsernameAndPassword obfuscatedCredentials;
        obfuscatedCredentials.keyId = keyId;
        obfuscatedCredentials.bytes = obfuscatedBytes;

        auto credentialsOrNull =
            NetworkProtocol::deobfuscateScrobblingCredentials(obfuscatedCredentials);

        if (credentialsOrNull == null)
        {
            qDebug() << "failed to deobfuscate scrobbling credentials";
            return;
        }

        auto future =
            _serverInterface->authenticateScrobblingProvider(
                provider,
                credentialsOrNull.value().username,
                credentialsOrNull.value().password
            );

        future.addResultListener(
            this,
            [this, clientReference](Result result)
            {
                sendResultMessage(result, clientReference);
            }
        );
    }

    void ConnectedClient::parseGeneratorNonRepetitionChangeMessage(
                                                                const QByteArray& message)
    {
        if (message.length() != 6)
            return; /* invalid message */

        if (!isLoggedIn()) return; /* client needs to be authenticated for this */

        qint32 intervalSeconds = NetworkUtil::get4BytesSigned(message, 2);
        qDebug() << "received change request for generator non-repetition interval;"
                 << "seconds:" << intervalSeconds;

        if (intervalSeconds < 0)
            return; /* invalid message */

        _serverInterface->setTrackRepetitionAvoidanceSeconds(intervalSeconds);
    }

    void ConnectedClient::parseCollectionFetchRequestMessage(const QByteArray& message)
    {
        if (message.length() != 8)
            return; /* invalid message */

        quint32 clientReference = NetworkUtil::get4Bytes(message, 4);

        qDebug() << "received collection fetch request; client ref:" << clientReference;

        if (!isLoggedIn())
        {
            /* client needs to be authenticated for this */
            sendResultMessage(ResultMessageErrorCode::NotLoggedIn, clientReference);
            return;
        }

        handleCollectionFetchRequest(clientReference);
    }

    void ConnectedClient::handleSingleByteAction(quint8 action)
    {
        /* actions 100-200 represent a SET VOLUME command */
        if (action >= 100 && action <= 200)
        {
            qDebug() << "received CHANGE VOLUME command, volume"
                     << (uint(action - 100));

            _serverInterface->setVolume(action - 100);
            return;
        }

        /* Other actions */

        switch (action)
        {
        case 1:
            qDebug() << "received PLAY command";
            _serverInterface->play();
            break;
        case 2:
            qDebug() << "received PAUSE command";
            _serverInterface->pause();
            break;
        case 3:
            qDebug() << "received SKIP command";
            _serverInterface->skip();
            break;
        case 4:
            qDebug() << "received INSERT BREAK AT FRONT command";
            _serverInterface->insertBreakAtFrontIfNotExists();
            break;
        case 10: /* request for state info */
            qDebug() << "received request for player status";
            sendPlayerStateMessage();
            break;
        case 11: /* request for status of dynamic mode */
            qDebug() << "received request for dynamic mode status";
            _serverInterface->requestDynamicModeStatus();
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
        case 16:
            qDebug() << "received request for server name";
            sendServerNameMessage();
            break;
        case 17:
            qDebug() << "received request for database UUID";
            sendDatabaseIdentifier();
            break;
        case 18:
            qDebug() << "received request for list of protocol extensions";
            sendProtocolExtensionsMessage();
            break;
        case 19:
            qDebug() << "received request for delayed start info";
            if (_serverInterface->delayedStartActive())
                sendDelayedStartInfoMessage();
            break;
        case 20: /* enable dynamic mode */
            qDebug() << "received ENABLE DYNAMIC MODE command";
            _serverInterface->enableDynamicMode();
            break;
        case 21: /* disable dynamic mode */
            qDebug() << "received DISABLE DYNAMIC MODE command";
            _serverInterface->disableDynamicMode();
            break;
        case 22: /* request queue expansion */
            qDebug() << "received QUEUE EXPANSION command";
            _serverInterface->requestQueueExpansion();
            break;
        case 23:
            qDebug() << "received TRIM QUEUE command";
            _serverInterface->trimQueue();
            break;
        case 24:
            qDebug() << "received START WAVE command";
            _serverInterface->startDynamicModeWave();
            break;
        case 25:
            qDebug() << "received TERMINATE WAVE command";
            _serverInterface->terminateDynamicModeWave();
            break;
        case 30: /* switch to public mode */
            qDebug() << "received SWITCH TO PUBLIC MODE command";
            _serverInterface->switchToPublicMode();
            break;
        case 31: /* switch to personal mode */
            qDebug() << "received SWITCH TO PERSONAL MODE command";
            _serverInterface->switchToPersonalMode();
            break;
        case 40:
            qDebug() << "received START FULL INDEXATION command";
            _serverInterface->startFullIndexation();
            break;
        case 50:
            qDebug() << "received SUBSCRIBE TO ALL EVENTS command";
            enableEvents();
            break;
        case 51:
            qDebug() << "received SUBSCRIBE TO SERVER HEALTH UPDATES command";
            enableHealthEvents(GeneralOrSpecific::Specific);
            break;
        case 60:
            qDebug() << "received request for server version information";
            sendServerVersionInfoMessage();
            break;
        case 99:
            qDebug() << "received SHUTDOWN command";
            _serverInterface->shutDownServer();
            break;
        default:
            qDebug() << "received unrecognized single-byte action type:"
                     << int(action);
            break; /* unknown action type */
        }
    }

    void ConnectedClient::handleParameterlessAction(ParameterlessActionCode code,
                                                    quint32 clientReference)
    {
        switch (code)
        {
        case ParameterlessActionCode::Reserved:
            break; /* not to be used, treat as invalid */

        case ParameterlessActionCode::ReloadServerSettings:
        {
            auto future = _serverInterface->reloadServerSettings();
            future.addResultListener(
                this,
                [this, clientReference](ResultMessageErrorCode error)
                {
                    Q_EMIT serverSettingsReloadResultEvent(clientReference, error);
                }
            );

            return;
        }
        case ParameterlessActionCode::DeactivateDelayedStart:
            {
                auto result = _serverInterface->deactivateDelayedStart();
                sendResultMessage(result, clientReference);
                return;
            }
        }

        qDebug() << "code of parameterless action not recognized: code="
                 << static_cast<int>(code) << " client-ref=" << clientReference;

        sendResultMessage(ResultMessageErrorCode::UnknownAction, clientReference);
    }

    void ConnectedClient::handleCollectionFetchRequest(uint clientReference)
    {
        auto sender =
            new CollectionSender(this, clientReference, &_player->resolver());

        connect(sender, &CollectionSender::sendCollectionList,
                this, &ConnectedClient::onCollectionTrackInfoBatchToSend);

        connect(sender, &CollectionSender::allSent,
                this, &ConnectedClient::onCollectionTrackInfoCompleted);
    }

    /* =============================== CollectionSender =============================== */

    CollectionSender::CollectionSender(ConnectedClient* connection, uint clientReference,
                                       Resolver *resolver)
     : QObject(connection), _clientRef(clientReference), _resolver(resolver),
       _currentIndex(0)
    {
        _hashes = _resolver->getAllHashes();
        qDebug() << "CollectionSender: starting.  Hash count:" << _hashes.size();

        QTimer::singleShot(0, this, &CollectionSender::sendNextBatch);
    }

    void CollectionSender::sendNextBatch()
    {
        if (_currentIndex >= _hashes.size())
        {
            qDebug() << "CollectionSender: all completed.  ref=" << _clientRef;
            Q_EMIT allSent(_clientRef);
            return;
        }

        int batchSize = qMin(30, _hashes.size());
        batchSize = qMin(batchSize, _hashes.size() - _currentIndex);

        auto batch = _hashes.mid(_currentIndex, batchSize);
        _currentIndex += batchSize;

        auto infoToSend = _resolver->getHashesTrackInfo(batch);
        qDebug() << "CollectionSender: have batch of" << infoToSend.size()
                 << "to send.  ref=" << _clientRef;

        /* schedule next batch already */
        QTimer::singleShot(100, this, &CollectionSender::sendNextBatch);

        /* send this batch if it is not empty */
        if (!infoToSend.isEmpty())
        {
            Q_EMIT sendCollectionList(_clientRef, infoToSend);
        }
    }

}
