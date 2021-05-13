/*
    Copyright (C) 2014-2021, Kevin Andre <hyperquantum@gmail.com>

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
#include "compatibilityuicontrollers.h"
#include "database.h"
#include "history.h"
#include "player.h"
#include "playerqueue.h"
#include "queueentry.h"
#include "resolver.h"
#include "server.h"
#include "serverhealthmonitor.h"
#include "serverinterface.h"
#include "users.h"

#include <QHostInfo>
#include <QMap>
#include <QThreadPool>
#include <QTimer>

namespace PMP
{
    /* ====================== ConnectedClient ====================== */

    const qint16 ConnectedClient::ServerProtocolNo = 15;

    ConnectedClient::ConnectedClient(QTcpSocket* socket, ServerInterface* serverInterface,
                                     Player* player,
                                     History* history,
                                     Users* users,
                                     CollectionMonitor* collectionMonitor,
                                     ServerHealthMonitor* serverHealthMonitor)
     : _socket(socket), _serverInterface(serverInterface),
       _player(player), _history(history),
       _users(users), _collectionMonitor(collectionMonitor),
       _serverHealthMonitor(serverHealthMonitor),
       _compatibilityUis(new CompatibilityUiControllerCollection(this, serverInterface)),
       _clientProtocolNo(-1),
       _lastSentNowPlayingID(0),
       _terminated(false), _binaryMode(false),
       _eventsEnabled(false),
       _healthEventsEnabled(false),
       _compatibilityUiEventsEnabled(false),
       _pendingPlayerStatus(false)
    {
        _serverInterface->setParent(this);

        connect(
            _serverInterface, &ServerInterface::serverShuttingDown,
            this, &ConnectedClient::terminateConnection
        );

        connect(
            socket, &QTcpSocket::disconnected,
            this, &ConnectedClient::terminateConnection
        );
        connect(socket, &QTcpSocket::readyRead, this, &ConnectedClient::dataArrived);
        connect(
            socket,
            static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(
                                                                      &QTcpSocket::error),
            this, &ConnectedClient::socketError
        );

        /* Send greeting.
         * The space at the end allows the client to detect that this server supports the
         * new 'binary' text command with one argument. A server whose greeting ends with
         * "Welcome!" without space will likely only support the 'binary' command without
         * argument. Of course, the greeting could also be changed to something completely
         * different from "Welcome!", in which case the space at the end will no longer be
         * needed. */
        sendTextCommand("PMP " PMP_VERSION_DISPLAY " Welcome! ");
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

    void ConnectedClient::enableEvents()
    {
        if (_eventsEnabled)
            return;

        qDebug() << "ConnectedClient: enabling event notifications";

        enableHealthEvents(GeneralOrSpecific::General);
        enableCompatibilityInterfaceEvents(GeneralOrSpecific::General);

        _eventsEnabled = true;

        auto queue = &_player->queue();

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
            queue, &PlayerQueue::entryAdded,
            this, &ConnectedClient::queueEntryAdded
        );
        connect(
            queue, &PlayerQueue::entryMoved,
            this, &ConnectedClient::queueEntryMoved
        );

        connect(
            _serverInterface, &ServerInterface::fullIndexationRunStatusChanged,
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
            _history, &History::updatedHashUserStats,
            this, &ConnectedClient::onUserHashStatsUpdated
        );
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

    void ConnectedClient::enableCompatibilityInterfaceEvents(GeneralOrSpecific howEnabled)
    {
        auto theseEventsWereEnabledAlready =
                _eventsEnabled | _compatibilityUiEventsEnabled;

        if (howEnabled == GeneralOrSpecific::Specific)
            _compatibilityUiEventsEnabled = true;

        if (theseEventsWereEnabledAlready)
            return; /* nothing to do */

        connect(
            _compatibilityUis,
            &CompatibilityUiControllerCollection::captionOrDescriptionChanged,
            this,
            [this](int interfaceId)
            {
                auto language = UserInterfaceLanguage::English; // FIXME
                sendCompatibilityInterfaceTextUpdate(interfaceId, language);
            }
        );
        connect(
            _compatibilityUis, &CompatibilityUiControllerCollection::stateChanged,
            this,
            [this](int interfaceId)
            {
                sendCompatibilityInterfaceStateUpdate(interfaceId);
            }
        );
        connect(
            _compatibilityUis, &CompatibilityUiControllerCollection::actionCaptionChanged,
            this,
            [this](int interfaceId, int actionId)
            {
                auto language = UserInterfaceLanguage::English; // FIXME
                sendCompatibilityInterfaceActionTextUpdate(interfaceId, actionId,
                                                           language);
            }
        );
        connect(
            _compatibilityUis, &CompatibilityUiControllerCollection::actionStateChanged,
            this,
            [this](int interfaceId, int actionId)
            {
                sendCompatibilityInterfaceActionStateUpdate(interfaceId, actionId);
            }
        );

        activateCompatibilityInterfaces();
        sendCompatibilityInterfacesAnnouncement();
    }

    void ConnectedClient::activateCompatibilityInterfaces()
    {
        if (_clientProtocolNo < 15)
            return; /* client does not know about compatibility user interfaces */

        /* for test purposes, activate this unconditionally */
        _compatibilityUis->activateIndexationController();

        /* for debugging */
        _compatibilityUis->activateTestController();
    }

    bool ConnectedClient::isLoggedIn() const
    {
        return _serverInterface->isLoggedIn();
    }

    void ConnectedClient::dataArrived()
    {
        if (_terminated) {
            qDebug() << "dataArrived called on a terminated connection???";
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
            qDebug() << "client protocol version:" << _clientProtocolNo;

            /* Help out old clients that do not know that the client now has to explicitly
             * ask for event notifications. */
            if (_clientProtocolNo < 2)
                enableEvents();
        }

        readBinaryCommands();
    }

    void ConnectedClient::socketError(QAbstractSocket::SocketError error) {
        switch (error) {
            case QAbstractSocket::RemoteHostClosedError:
                this->terminateConnection();
                break;
            default:
                qDebug() << "ConnectedClient: unhandled socket error:" << error;
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

    void ConnectedClient::executeTextCommandWithoutArgs(const QString& command) {
        if (command == "play") {
            _serverInterface->play();
        }
        else if (command == "pause") {
            _serverInterface->pause();
        }
        else if (command == "skip") {
            _serverInterface->skip();
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
            _serverInterface->shutDownServer();
        }
        else if (command == "binary") {
            /* 'binary' command without argument is obsolete, but we need to continue
             * supporting it for some time in order to be compatible with older clients */
            qDebug() << " 'binary' command has obsolete form (needs argument)";
            handleBinaryModeSwitchRequest();
        }
        else {
            /* unknown command ???? */
            qDebug() << " unknown text command: " << command;
        }
    }

    void ConnectedClient::executeTextCommandWithArgs(const QString& command,
                                                     const QString& arg1)
    {
        if (command == "binary") {
            /* Requiring an argument that is (nearly) impossible to remember will prevent
             * an accidental switch to binary mode when a person is typing commands. */
            if (arg1 == "NUxwyGR3ivTcB27VGYdy") {
                handleBinaryModeSwitchRequest();
            }
            else {
                qDebug() << " argument for 'binary' command not recognized";
            }
        }
        else if (command == "volume") {
            /* 'volume' with one argument is a request to changes the current volume */
            bool ok;
            int volume = arg1.toInt(&ok);
            if (ok && volume >= 0 && volume <= 100) {
                _serverInterface->setVolume(volume);
            }
        }
        else if (command == "shutdown") {
            _serverInterface->shutDownServer(arg1);
        }
        else {
            /* unknown command ???? */
            qDebug() << " unknown text command: " << command;
        }
    }

    void ConnectedClient::executeTextCommandWithArgs(const QString& command,
                                                     const QString& arg1,
                                                     const QString& arg2)
    {
        if (command == "qmove") {
            bool ok;
            uint queueID = arg1.toUInt(&ok);
            if (!ok || queueID == 0) return;

            if (!arg2.startsWith("+") && !arg2.startsWith("-")) return;
            int moveDiff = arg2.toInt(&ok);
            if (!ok || moveDiff == 0) return;

            _serverInterface->moveQueueEntry(queueID, moveDiff);
        }
        else {
            /* unknown command ???? */
            qDebug() << " unknown text command: " << command;
        }
    }

    void ConnectedClient::executeTextCommand(QString const& commandText) {
        qDebug() << "received text commandline:" << commandText;

        int spaceIndex = commandText.indexOf(' ');

        if (spaceIndex < 0) { /* command without arguments */
            executeTextCommandWithoutArgs(commandText);
            return;
        }

        /* split command at the space; don't include the space in the parts */
        QString command = commandText.left(spaceIndex);
        QString rest = commandText.mid(spaceIndex + 1);
        spaceIndex = rest.indexOf(' ');

        if (spaceIndex < 0) { /* one argument only */
            executeTextCommandWithArgs(command, rest);
            return;
        }

        /* two arguments or more */

        QString arg1 = rest.left(spaceIndex);
        rest = rest.mid(spaceIndex + 1);
        spaceIndex = rest.indexOf(' ');

        if (spaceIndex < 0) { /* two arguments only */
            executeTextCommandWithArgs(command, arg1, rest);
            return;
        }

        /* unknown command ???? */
        qDebug() << " unknown text command, or wrong number of arguments";
    }

    void ConnectedClient::sendTextCommand(QString const& command) {
        if (_terminated) {
            qDebug() << "cannot send text command because connection is terminated";
            return;
        }

        _socket->write((command + ";").toUtf8());
        _socket->flush();
    }

    void ConnectedClient::handleBinaryModeSwitchRequest() {
        if (_binaryMode) {
            qDebug() << "cannot switch to binary mode, already in it";
            return;
        }

        /* switch to binary mode */
        _binaryMode = true;
        /* tell the client that all further communication will be in binary mode */
        sendTextCommand("binary");

        char binaryHeader[5];
        binaryHeader[0] = 'P';
        binaryHeader[1] = 'M';
        binaryHeader[2] = 'P';
        /* the next two bytes are the protocol version */
        binaryHeader[3] = char(((unsigned)ServerProtocolNo >> 8) & 255);
        binaryHeader[4] = char((unsigned)ServerProtocolNo & 255);
        _socket->write(binaryHeader, sizeof(binaryHeader));
        _socket->flush();
    }

    void ConnectedClient::sendBinaryMessage(QByteArray const& message) {
        if (_terminated) {
            qDebug() << "cannot send binary message because connection is terminated";
            return;
        }

        if (!_binaryMode) {
            qDebug() << "cannot send binary message when not in binary mode";
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

    void ConnectedClient::sendProtocolExtensionsMessage() {
        if (_clientProtocolNo < 12) return; /* client will not understand this message */

        QVector<const NetworkProtocol::ProtocolExtension*> extensions;
        //extensions << &_knownExtensionThis;

        quint8 extensionCount = static_cast<quint8>(extensions.size());

        QByteArray message;
        message.reserve(4 + extensionCount * 16); /* estimate */
        NetworkUtil::append2Bytes(message, NetworkProtocol::ServerExtensionsMessage);
        NetworkUtil::appendByte(message, 0); /* filler */
        NetworkUtil::appendByte(message, extensionCount);

        for(auto extension : extensions) {
            QByteArray nameBytes = extension->name.toUtf8();
            quint8 nameBytesCount = static_cast<quint8>(nameBytes.size());

            NetworkUtil::appendByte(message, extension->id);
            NetworkUtil::appendByte(message, extension->version);
            NetworkUtil::appendByte(message, nameBytesCount);
            message += nameBytes;
        }

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendStateInfoAfterTimeout() {
        _pendingPlayerStatus = false;
        sendStateInfo();
    }

    void ConnectedClient::sendStateInfo() {
        //qDebug() << "sending state info";

        ServerPlayerState state = _player->state();
        quint8 stateNum = 0;
        switch (state) {
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

    void ConnectedClient::sendDynamicModeStatusMessage(StartStopEventStatus enabledStatus,
                                                       int noRepetitionSpanSeconds)
    {
        quint8 enabled = Common::isActive(enabledStatus) ? 1 : 0;
        qint32 noRepetitionSpan = noRepetitionSpanSeconds;

        QByteArray message;
        message.reserve(7);
        NetworkUtil::append2Bytes(message, NetworkProtocol::DynamicModeStatusMessage);
        NetworkUtil::appendByte(message, enabled);
        NetworkUtil::append4BytesSigned(message, noRepetitionSpan);

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendUserPlayingForModeMessage() {
        quint32 user = _player->userPlayingFor();
        QString login = _users->getUserLogin(user);
        QByteArray loginBytes = login.toUtf8();

        QByteArray message;
        message.reserve(4 + 4 + loginBytes.length());
        NetworkUtil::append2Bytes(message, NetworkProtocol::UserPlayingForModeMessage);
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
        NetworkUtil::append2Bytes(message, NetworkProtocol::DynamicModeWaveStatusMessage);
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

    void ConnectedClient::sendEventNotificationMessage(quint8 event) {
        qDebug() << "   sending server event number" << event;

        QByteArray message;
        message.reserve(2 + 2);
        NetworkUtil::append2Bytes(
            message, NetworkProtocol::ServerEventNotificationMessage
        );
        NetworkUtil::appendByte(message, event);
        NetworkUtil::appendByte(message, 0); /* unused */

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendServerInstanceIdentifier() {
        QUuid uuid = _serverInterface->getServerUuid();

        QByteArray message;
        message.reserve(2 + 16);
        NetworkUtil::append2Bytes(
            message, NetworkProtocol::ServerInstanceIdentifierMessage
        );
        message.append(uuid.toRfc4122());

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendDatabaseIdentifier() {
        auto db = Database::getDatabaseForCurrentThread();
        if (!db) {
            return; /* database unusable */
        }

        QUuid uuid = db->getDatabaseIdentifier();

        QByteArray message;
        message.reserve(2 + 16);
        NetworkUtil::append2Bytes(message, NetworkProtocol::DatabaseIdentifierMessage);
        message.append(uuid.toRfc4122());

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendUsersList() {
        QList<UserIdAndLogin> users = _users->getUsers();
        auto usersCount = users.size();
        if (usersCount > std::numeric_limits<quint16>::max()) {
            qWarning() << "users count exceeds limit, cannot send list";
            return;
        }

        QByteArray message;
        message.reserve(2 + 2 + users.size() * (4 + 1 + 20)); /* only an approximation */
        NetworkUtil::append2Bytes(message, NetworkProtocol::UsersListMessage);
        NetworkUtil::append2Bytes(message, static_cast<quint16>(usersCount));

        Q_FOREACH(UserIdAndLogin user, users) {
            NetworkUtil::append4Bytes(message, user.first); /* ID */

            QByteArray loginNameBytes = user.second.toUtf8();
            loginNameBytes.truncate(255);

            NetworkUtil::appendByte(message, static_cast<quint8>(loginNameBytes.size()));
            message += loginNameBytes;
        }

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendQueueContentMessage(quint32 startOffset, quint8 length) {
        PlayerQueue& queue = _player->queue();
        uint queueLength = queue.length();

        if (startOffset >= queueLength) {
            length = 0;
        }
        else if (startOffset > queueLength - length) {
            length = queueLength - startOffset;
        }

        QList<QueueEntry*> entries =
            queue.entries(startOffset, (length == 0) ? -1 : length);

        QByteArray message;
        message.reserve(10 + entries.length() * 4);
        NetworkUtil::append2Bytes(message, NetworkProtocol::QueueContentsMessage);
        NetworkUtil::append4Bytes(message, queueLength);
        NetworkUtil::append4Bytes(message, startOffset);

        Q_FOREACH(QueueEntry* entry, entries) {
            NetworkUtil::append4Bytes(message, entry->queueID());
        }

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendQueueHistoryMessage(int limit) {
        PlayerQueue& queue = _player->queue();

        /* keep a reasonable limit */
        if (limit <= 0 || limit > 255) { limit = 255; }

        auto entries = queue.recentHistory(limit);
        auto entryCount = static_cast<quint8>(entries.length());

        QByteArray message;
        message.reserve(2 + 1 + 1 + entryCount * 28);
        NetworkUtil::append2Bytes(message, NetworkProtocol::PlayerHistoryMessage);
        NetworkUtil::appendByte(message, 0); /* filler */
        NetworkUtil::appendByte(message, entryCount);

        Q_FOREACH(auto entry, entries) {
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

    void ConnectedClient::sendQueueEntryAdditionConfirmationMessage(
            quint32 clientReference, quint32 index, quint32 queueID)
    {
        QByteArray message;
        message.reserve(16);
        NetworkUtil::append2Bytes(message,
                                  NetworkProtocol::QueueEntryAdditionConfirmationMessage);
        NetworkUtil::append2Bytes(message, 0); /* filler */
        NetworkUtil::append4Bytes(message, clientReference);
        NetworkUtil::append4Bytes(message, index);
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

    void ConnectedClient::sendQueueEntryInfoMessage(quint32 queueID) {
        QueueEntry* track = _player->queue().lookup(queueID);
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
        NetworkUtil::append2Bytes(message, NetworkProtocol::TrackInfoMessage);
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

    void ConnectedClient::sendQueueEntryInfoMessage(QList<quint32> const& queueIDs) {
        if (queueIDs.empty()) {
            return;
        }

        const int maxSize = (1 << 16) - 1;

        /* not too big? */
        if (queueIDs.size() > maxSize) {
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

        NetworkUtil::append2Bytes(message, NetworkProtocol::BulkTrackInfoMessage);
        NetworkUtil::append2Bytes(message, (uint)queueIDs.size());

        PlayerQueue& queue = _player->queue();

        /* TODO: bug: concurrency issue here when a QueueEntry has just been deleted */

        for(quint32 queueID : queueIDs)
        {
            QueueEntry* track = queue.lookup(queueID);
            auto trackStatus =
                track ? createTrackStatusFor(track)
                      : NetworkProtocol::createTrackStatusUnknownId();
            NetworkUtil::append2Bytes(message, trackStatus);
        }
        if (queueIDs.size() % 2 != 0) {
            NetworkUtil::append2Bytes(message, 0); /* filler */
        }

        for(quint32 queueID : queueIDs)
        {
            QueueEntry* track = queue.lookup(queueID);
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

    void ConnectedClient::sendQueueEntryHashMessage(const QList<quint32> &queueIDs) {
        if (queueIDs.empty()) {
            return;
        }

        const int maxSize = (1 << 16) - 1;

        /* not too big? */
        if (queueIDs.size() > maxSize) {
            /* TODO: maybe delay the second part? */
            sendQueueEntryInfoMessage(queueIDs.mid(0, maxSize));
            sendQueueEntryInfoMessage(queueIDs.mid(maxSize));
            return;
        }

        QByteArray message;
        message.reserve(4 + queueIDs.size() * (8 + NetworkProtocol::FILEHASH_BYTECOUNT));

        NetworkUtil::append2Bytes(message, NetworkProtocol::BulkQueueEntryHashMessage);
        NetworkUtil::append2Bytes(message, (uint)queueIDs.size());

        PlayerQueue& queue = _player->queue();

        /* TODO: bug: concurrency issue here when a QueueEntry has just been deleted */

        FileHash emptyHash;

        Q_FOREACH(quint32 queueID, queueIDs) {
            QueueEntry* track = queue.lookup(queueID);
            auto trackStatus =
                track ? createTrackStatusFor(track)
                      : NetworkProtocol::createTrackStatusUnknownId();

            const FileHash* hash = track ? track->hash() : &emptyHash;
            hash = hash ? hash : &emptyHash;

            NetworkUtil::append4Bytes(message, queueID);
            NetworkUtil::append2Bytes(message, trackStatus);
            NetworkUtil::append2Bytes(message, 0); /* filler */
            NetworkProtocol::appendHash(message, *hash);
        }

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendPossibleTrackFilenames(quint32 queueID,
                                                     QList<QString> const& names)
    {
        QByteArray message;
        message.reserve(2 + 4 + names.size() * (4 + 30)); /* only an approximation */
        NetworkUtil::append2Bytes(
            message, NetworkProtocol::PossibleFilenamesForQueueEntryMessage
        );

        NetworkUtil::append4Bytes(message, queueID);

        Q_FOREACH(QString name, names) {
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
        if (available.size() > maxSize) {
            /* TODO: maybe delay the second part? */
            sendTrackAvailabilityBatchMessage(available.mid(0, maxSize), unavailable);
            sendTrackAvailabilityBatchMessage(available.mid(maxSize), unavailable);
            return;
        }
        if (unavailable.size() > maxSize) {
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

        NetworkUtil::append2Bytes(message,
                        NetworkProtocol::CollectionAvailabilityChangeNotificationMessage);
        NetworkUtil::append2Bytes(message, 0) /* filler */;
        NetworkUtil::append2Bytes(message, available.size());
        NetworkUtil::append2Bytes(message, unavailable.size());

        for (int i = 0; i < available.size(); ++i) {
            NetworkProtocol::appendHash(message, available[i]);
        }

        for (int i = 0; i < unavailable.size(); ++i) {
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
        if (tracks.size() > maxSize) {
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

        NetworkUtil::append2Bytes(
            message,
            isNotification ? NetworkProtocol::CollectionChangeNotificationMessage
                           : NetworkProtocol::CollectionFetchResponseMessage
        );
        NetworkUtil::append2Bytes(message, tracks.size());
        if (!isNotification) {
            NetworkUtil::append4Bytes(message, clientReference);
        }

        for (int i = 0; i < tracks.size(); ++i) {
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
            if (withAlbumAndTrackLength) {
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

        NetworkUtil::append2Bytes(message, NetworkProtocol::NewHistoryEntryMessage);
        NetworkUtil::append2Bytes(message, 0); /* filler */
        NetworkUtil::append4Bytes(message, entry->queueID());
        NetworkUtil::append4Bytes(message, entry->user());
        NetworkUtil::append8ByteQDateTimeMsSinceEpoch(message, entry->started());
        NetworkUtil::append8ByteQDateTimeMsSinceEpoch(message, entry->ended());
        NetworkUtil::append2BytesSigned(message, entry->permillage());
        NetworkUtil::append2Bytes(message, status);

        sendBinaryMessage(message);
    }

    void ConnectedClient::userDataForHashesFetchCompleted(quint32 userId,
                                                         QVector<UserDataForHash> results,
                                                          bool havePreviouslyHeard,
                                                          bool haveScore)
    {
        const int maxSize = (1 << 16) - 1;

        /* not too big? */
        if (results.size() > maxSize) {
            /* TODO: maybe delay the second part? */
            userDataForHashesFetchCompleted(
                userId, results.mid(0, maxSize), havePreviouslyHeard, haveScore
            );
            userDataForHashesFetchCompleted(
                userId, results.mid(maxSize), havePreviouslyHeard, haveScore
            );
            return;
        }

        qint16 fields = (havePreviouslyHeard ? 1 : 0) | (haveScore ? 2 : 0);
        fields &=
            NetworkProtocol::getHashUserDataFieldsMaskForProtocolVersion(
                                                                       _clientProtocolNo);
        uint fieldsSize = (havePreviouslyHeard ? 8 : 0) + (haveScore ? 2 : 0);

        qDebug() << "sending user data for" << results.size()
                 << "hashes; fields:" << fields;

        QByteArray message;
        message.reserve(
            2 + 2 + 4 + 4
                + results.size() * (NetworkProtocol::FILEHASH_BYTECOUNT + fieldsSize)
        );

        NetworkUtil::append2Bytes(message, NetworkProtocol::HashUserDataMessage);
        NetworkUtil::append2Bytes(message, results.size());
        NetworkUtil::append2Bytes(message, 0); /* filler */
        NetworkUtil::append2Bytes(message, fields);
        NetworkUtil::append4Bytes(message, userId);

        Q_FOREACH(auto const& result, results) {
            NetworkProtocol::appendHash(message, result.hash);

            if (havePreviouslyHeard) {
                NetworkUtil::append8ByteMaybeEmptyQDateTimeMsSinceEpoch(
                    message, result.previouslyHeard
                );
            }

            if (haveScore) {
                NetworkUtil::append2Bytes(message, (quint16)result.score);
            }
        }

        sendBinaryMessage(message);
    }

    void ConnectedClient::onHashAvailabilityChanged(QVector<FileHash> available,
                                                    QVector<FileHash> unavailable)
    {
        sendTrackAvailabilityBatchMessage(available, unavailable);
    }

    void ConnectedClient::onHashInfoChanged(QVector<CollectionTrackInfo> changes) {
        sendTrackInfoBatchMessage(0, true, changes);
    }

    void ConnectedClient::onCollectionTrackInfoCompleted(uint clientReference) {
        sendSuccessMessage(clientReference, 0);
    }

    void ConnectedClient::sendServerNameMessage(quint8 type, QString name) {
        name.truncate(63);
        QByteArray nameBytes = name.toUtf8();

        QByteArray message;
        message.reserve(2 + 2 + nameBytes.size());
        NetworkUtil::append2Bytes(message, NetworkProtocol::ServerNameMessage);
        NetworkUtil::appendByte(message, 0); /* unused */
        NetworkUtil::appendByte(message, type);
        message += nameBytes;

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendServerHealthMessageIfNotEverythingOkay() {
        if (!_serverHealthMonitor->anyProblem()) return;

        sendServerHealthMessage();
    }

    void ConnectedClient::sendServerHealthMessage() {
        /* only send it if the client will understand it */
        if (_clientProtocolNo < 10) return;

        quint16 problems = 0;
        problems |= _serverHealthMonitor->databaseUnavailable() ? 1u : 0u;

        QByteArray message;
        message.reserve(2 + 2);
        NetworkUtil::append2Bytes(message, NetworkProtocol::ServerHealthMessage);
        NetworkUtil::append2Bytes(message, problems);

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendCompatibilityInterfacesAnnouncement()
    {
        /* only send it if the client will understand it */
        if (_clientProtocolNo < 15) return;

        auto const ids = _compatibilityUis->getControllerIds();
        if (ids.isEmpty())
            return; /* nothing to announce */

        qDebug() << "sending compatibility interfaces announcement";

        QByteArray message;
        message.reserve(2 + 2 + 2 * ids.size() + 2);
        NetworkUtil::append2Bytes(message,
                                  NetworkProtocol::CompatibilityInterfaceAnnouncement);
        NetworkUtil::append2BytesUnsigned(message, ids.size());

        for (auto id : ids)
        {
            NetworkUtil::append2BytesUnsigned(message, id);
        }

        if (ids.size() % 2 != 0) // padding
        {
            NetworkUtil::append2Bytes(message, 0);
        }

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendCompatibilityInterfaceDefinition(int interfaceId,
                                                           UserInterfaceLanguage language)
    {
        /* only send it if the client will understand it */
        if (_clientProtocolNo < 15) return;

        auto* controller = _compatibilityUis->getControllerById(interfaceId);
        if (!controller)
            return;

        qDebug() << "sending compatibility interface definition for ID" << interfaceId;

        quint16 encodedState =
                NetworkProtocol::encodeCompatibilityUiState(controller->getState());

        auto const actionIds = controller->getActionIds();

        auto interfaceTitle = controller->getTitle(language);
        auto interfaceText = controller->getText(language);
        auto languageCode = NetworkProtocol::encodeLanguage(language);

        auto interfaceTitleBytes = NetworkUtil::getUtf8Bytes(interfaceTitle, 255);
        auto interfaceCaptionBytes =
                NetworkUtil::getUtf8Bytes(interfaceText.caption, 1023);
        auto interfaceDescriptionBytes =
                NetworkUtil::getUtf8Bytes(interfaceText.description, 0xFFFF);

        QByteArray message;
        message.reserve(2 + 2 + 2 + 2 + 2 + 2 + 4
                        + interfaceTitleBytes.size()
                        + interfaceCaptionBytes.size()
                        + interfaceDescriptionBytes.size()
                        + actionIds.size() * (6 + 26 /*estimate*/));
        NetworkUtil::append2Bytes(message,
                                  NetworkProtocol::CompatibilityInterfaceDefinition);
        NetworkUtil::append2BytesUnsigned(message, interfaceId);
        NetworkUtil::append2BytesUnsigned(message, encodedState);
        NetworkUtil::append2BytesUnsigned(message, actionIds.size());
        NetworkUtil::append2BytesUnsigned(message, interfaceCaptionBytes.size());
        NetworkUtil::append2BytesUnsigned(message, interfaceDescriptionBytes.size());
        NetworkUtil::appendByteUnsigned(message, interfaceTitleBytes.size());
        NetworkUtil::appendByte(message, languageCode[0]);
        NetworkUtil::appendByte(message, languageCode[1]);
        NetworkUtil::appendByte(message, languageCode[2]);

        message.append(interfaceTitleBytes);
        message.append(interfaceCaptionBytes);
        message.append(interfaceDescriptionBytes);

        for (auto actionId : actionIds)
        {
            auto actionState = controller->getActionState(actionId);
            auto actionCaption = controller->getActionCaption(actionId, language);

            quint8 encodedActionState =
                    NetworkProtocol::encodeCompatibilityUiActionState(actionState);

            auto actionCaptionBytes = NetworkUtil::getUtf8Bytes(actionCaption, 255);

            NetworkUtil::append2BytesUnsigned(message, actionId);
            NetworkUtil::appendByte(message, 0); // filler
            NetworkUtil::appendByte(message, encodedActionState);
            NetworkUtil::append2BytesUnsigned(message, actionCaptionBytes.size());

            message.append(actionCaptionBytes);
        }

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendCompatibilityInterfaceStateUpdate(int interfaceId)
    {
        /* only send it if the client will understand it */
        if (_clientProtocolNo < 15) return;

        auto* controller = _compatibilityUis->getControllerById(interfaceId);
        if (!controller)
            return;

        qDebug() << "sending compatibility interface state update for ID" << interfaceId;

        quint16 encodedState =
                NetworkProtocol::encodeCompatibilityUiState(controller->getState());

        QByteArray message;
        message.reserve(2 + 2 + 2 + 2);
        NetworkUtil::append2Bytes(message,
                                  NetworkProtocol::CompatibilityInterfaceStateUpdate);
        NetworkUtil::append2Bytes(message, 0); // padding
        NetworkUtil::append2BytesUnsigned(message, interfaceId);
        NetworkUtil::append2Bytes(message, encodedState);

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendCompatibilityInterfaceActionStateUpdate(int interfaceId,
                                                                      int actionId)
    {
        /* only send it if the client will understand it */
        if (_clientProtocolNo < 15) return;

        auto* controller = _compatibilityUis->getControllerById(interfaceId);
        if (!controller)
            return;

        qDebug() << "sending compatibility interface action state update for interface"
                 << interfaceId << "and action" << actionId;

        auto actionState = controller->getActionState(actionId);
        quint8 encodedActionState =
                NetworkProtocol::encodeCompatibilityUiActionState(actionState);

        QByteArray message;
        message.reserve(2 + 2 + 2 + 1 + 1);
        NetworkUtil::append2Bytes(message,
                                NetworkProtocol::CompatibilityInterfaceActionStateUpdate);
        NetworkUtil::append2BytesUnsigned(message, interfaceId);
        NetworkUtil::append2BytesUnsigned(message, actionId);
        NetworkUtil::appendByte(message, 0); // filler
        NetworkUtil::appendByte(message, encodedActionState);

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendCompatibilityInterfaceTextUpdate(int interfaceId,
                                                           UserInterfaceLanguage language)
    {
        /* only send it if the client will understand it */
        if (_clientProtocolNo < 15) return;

        auto* controller = _compatibilityUis->getControllerById(interfaceId);
        if (!controller)
            return;

        qDebug() << "sending compatibility interface text update for ID" << interfaceId;

        auto interfaceText = controller->getText(language);
        auto languageCode = NetworkProtocol::encodeLanguage(language);

        auto interfaceCaptionBytes =
                NetworkUtil::getUtf8Bytes(interfaceText.caption, 1023);
        auto interfaceDescriptionBytes =
                NetworkUtil::getUtf8Bytes(interfaceText.description, 0xFFFF);

        QByteArray message;
        message.reserve(2 + 2 + 2 + 2 + 4 + interfaceCaptionBytes.size()
                                          + interfaceDescriptionBytes.size());
        NetworkUtil::append2Bytes(message,
                                  NetworkProtocol::CompatibilityInterfaceTextUpdate);
        NetworkUtil::append2BytesUnsigned(message, interfaceId);
        NetworkUtil::append2BytesUnsigned(message, interfaceCaptionBytes.size());
        NetworkUtil::append2BytesUnsigned(message, interfaceDescriptionBytes.size());
        NetworkUtil::appendByte(message, 0); // filler
        NetworkUtil::appendByte(message, languageCode[0]);
        NetworkUtil::appendByte(message, languageCode[1]);
        NetworkUtil::appendByte(message, languageCode[2]);

        message.append(interfaceCaptionBytes);
        message.append(interfaceDescriptionBytes);

        sendBinaryMessage(message);
    }

    void ConnectedClient::sendCompatibilityInterfaceActionTextUpdate(int interfaceId,
                                                                     int actionId,
                                                           UserInterfaceLanguage language)
    {
        /* only send it if the client will understand it */
        if (_clientProtocolNo < 15) return;

        auto* controller = _compatibilityUis->getControllerById(interfaceId);
        if (!controller)
            return;

        qDebug() << "sending compatibility interface action text update for interface"
                 << interfaceId << "and action" << actionId;

        auto actionCaption = controller->getActionCaption(actionId, language);
        auto languageCode = NetworkProtocol::encodeLanguage(language);

        auto actionCaptionBytes = NetworkUtil::getUtf8Bytes(actionCaption, 255);

        QByteArray message;
        message.reserve(2 + 2 + 2 + 2 + 4 + actionCaptionBytes.size());
        NetworkUtil::append2Bytes(message,
                                  NetworkProtocol::CompatibilityInterfaceActionTextUpdate);
        NetworkUtil::append2BytesUnsigned(message, interfaceId);
        NetworkUtil::append2BytesUnsigned(message, actionId);
        NetworkUtil::append2BytesUnsigned(message, actionCaptionBytes.size());
        NetworkUtil::appendByte(message, 0); // filler
        NetworkUtil::appendByte(message, languageCode[0]);
        NetworkUtil::appendByte(message, languageCode[1]);
        NetworkUtil::appendByte(message, languageCode[2]);

        message.append(actionCaptionBytes);

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

    void ConnectedClient::sendNonFatalInternalErrorResultMessage(quint32 clientReference)
    {
        sendResultMessage(
                        NetworkProtocol::NonFatalInternalServerError, clientReference, 0);
    }

    void ConnectedClient::serverHealthChanged(bool databaseUnavailable) {
        (void)databaseUnavailable;

        sendServerHealthMessage();
    }

    void ConnectedClient::volumeChanged(int volume) {
        (void)volume;

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

    void ConnectedClient::onUserPlayingForChanged(quint32 user) {
        (void)user;

        sendUserPlayingForModeMessage();
    }

    void ConnectedClient::onUserHashStatsUpdated(uint hashID, quint32 user,
                                                 QDateTime previouslyHeard, qint16 score)
    {
        qDebug() << "ConnectedClient::onUserHashStatsUpdated; user:" << user
                 << " hash:" << hashID
                 << " prevHeard:" << previouslyHeard << " score:" << score;

        UserDataForHash data;
        data.hash = _player->resolver().getHashByID(hashID);
        if (data.hash.isNull()) return; /* invalid ID */

        data.previouslyHeard = previouslyHeard;
        data.score = score;

        QVector<UserDataForHash> dataList;
        dataList << data;
        userDataForHashesFetchCompleted(user, dataList, true, true);
    }

    void ConnectedClient::onFullIndexationRunStatusChanged(bool running) {
        sendEventNotificationMessage(running ? 1 : 2);
    }

    void ConnectedClient::playerStateChanged(ServerPlayerState state) {
        if (_binaryMode) {
            sendStateInfo();
            return;
        }

        switch (state) {
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

    void ConnectedClient::currentTrackChanged(QueueEntry const* entry) {
        if (_binaryMode) {
            sendStateInfo();
            return;
        }

        if (entry == nullptr) {
            sendTextCommand("nowplaying nothing");
            return;
        }

        int seconds = static_cast<int>(entry->lengthInMilliseconds() / 1000);
        FileHash const* hash = entry->hash();

        sendTextCommand(
            "nowplaying track\n QID: " + QString::number(entry->queueID())
             + "\n position: " + QString::number(_player->playPosition())
             + "\n title: " + entry->title()
             + "\n artist: " + entry->artist()
             + "\n length: " + (seconds < 0 ? "?" : QString::number(seconds)) + " sec"
             + "\n hash length: " + (!hash ? "?" : QString::number(hash->length()))
             + "\n hash SHA-1: " + (!hash ? "?" : hash->SHA1().toHex())
             + "\n hash MD5: " + (!hash ? "?" : hash->MD5().toHex())
        );
    }

    void ConnectedClient::newHistoryEntry(QSharedPointer<PlayerHistoryEntry> entry)
    {
        if (_binaryMode)
            sendNewHistoryEntryMessage(entry);
    }

    void ConnectedClient::trackPositionChanged(qint64 position) {
        (void)position;

        if (_binaryMode) {
            sendStateInfo();
            return;
        }

        //sendTextCommand("position " + QString::number(position));
    }

    void ConnectedClient::sendTextualQueueInfo() {
        PlayerQueue& queue = _player->queue();
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

            int lengthInSeconds =
                static_cast<int>(entry->lengthInMilliseconds() / 1000);

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

    void ConnectedClient::schedulePlayerStateNotification() {
        if (_pendingPlayerStatus) return;

        _pendingPlayerStatus = true;
        QTimer::singleShot(25, this, &ConnectedClient::sendStateInfoAfterTimeout);
    }

    void ConnectedClient::queueEntryRemoved(quint32 offset, quint32 queueID) {
        sendQueueEntryRemovedMessage(offset, queueID);
        schedulePlayerStateNotification(); /* queue length changed, notify after delay */
    }

    void ConnectedClient::queueEntryAdded(quint32 offset, quint32 queueID)
    {
        auto it = _trackAdditionConfirmationsPending.find(queueID);
        if (it != _trackAdditionConfirmationsPending.end()) {
            auto clientReference = it.value();
            _trackAdditionConfirmationsPending.erase(it);

            if (_clientProtocolNo >= 9) {
                sendQueueEntryAdditionConfirmationMessage(
                                                        clientReference, offset, queueID);
            }
            else {
                sendSuccessMessage(clientReference, queueID);
            }
        }
        else {
            sendQueueEntryAddedMessage(offset, queueID);
        }

        schedulePlayerStateNotification(); /* queue length changed, notify after delay */
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
        if (message.length() < 2) {
            qDebug() << "received invalid binary message (less than 2 bytes)";
            return; /* invalid message */
        }

        quint16 messageType = NetworkUtil::get2Bytes(message, 0);
        if (messageType & (1u << 15)) {
            quint8 extensionMessageType = messageType & 0x7Fu;
            quint8 extensionId = (messageType >> 7) & 0xFFu;

            handleExtensionMessage(extensionId, extensionMessageType, message);
        }
        else {
            auto clientMessageType =
                static_cast<NetworkProtocol::ClientMessageType>(messageType);

            handleStandardBinaryMessage(clientMessageType, message);
        }
    }

    void ConnectedClient::handleStandardBinaryMessage(
                                           NetworkProtocol::ClientMessageType messageType,
                                           QByteArray const& message)
    {
        qint32 messageLength = message.length();

        switch (messageType) {
        case NetworkProtocol::ClientExtensionsMessage:
            parseClientProtocolExtensionsMessage(message);
            break;
        case NetworkProtocol::SingleByteActionMessage:
        {
            if (messageLength != 3) {
                return; /* invalid message */
            }

            quint8 actionType = NetworkUtil::getByte(message, 2);
            handleSingleByteAction(actionType);
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

            sendQueueEntryInfoMessage(queueID);
        }
            break;
        case NetworkProtocol::BulkTrackInfoRequestMessage:
        {
            if (messageLength < 6 || (messageLength - 2) % 4 != 0) {
                return; /* invalid message */
            }

            QList<quint32> QIDs;
            QIDs.reserve((messageLength - 2) / 4);

            int offset = 2;
            while (offset <= messageLength - 4) {
                quint32 queueID = NetworkUtil::get4Bytes(message, offset);

                if (queueID > 0) {
                    QIDs.append(queueID);
                }

                offset += 4;
            }

            qDebug() << "received bulk track info request for" << QIDs.size() << "QIDs";

            sendQueueEntryInfoMessage(QIDs);
        }
            break;
        case NetworkProtocol::BulkQueueEntryHashRequestMessage:
        {
            if (messageLength < 8 || (messageLength - 4) % 4 != 0) {
                return; /* invalid message */
            }

            QList<quint32> QIDs;
            QIDs.reserve((messageLength - 4) / 4);

            int offset = 4;
            while (offset <= messageLength - 4) {
                quint32 queueID = NetworkUtil::get4Bytes(message, offset);

                if (queueID > 0) {
                    QIDs.append(queueID);
                }

                offset += 4;
            }

            qDebug() << "received bulk hash info request for" << QIDs.size() << "QIDs";

            sendQueueEntryHashMessage(QIDs);
        }
            break;
        case NetworkProtocol::QueueFetchRequestMessage:
        {
            if (messageLength != 7) {
                return; /* invalid message */
            }

            if (!isLoggedIn()) return; /* client needs to be authenticated for this */

            quint32 startOffset = NetworkUtil::get4Bytes(message, 2);
            quint8 length = NetworkUtil::getByte(message, 6);

            qDebug() << "received queue fetch request; offset:" << startOffset
                     << "  length:" << length;

            sendQueueContentMessage(startOffset, length);
        }
            break;
        case NetworkProtocol::QueueEntryRemovalRequestMessage:
            parseQueueEntryRemovalRequest(message);
            break;
        case NetworkProtocol::GeneratorNonRepetitionChangeMessage:
        {
            if (messageLength != 6) {
                return; /* invalid message */
            }

            if (!isLoggedIn()) return; /* client needs to be authenticated for this */

            qint32 intervalSeconds = NetworkUtil::get4BytesSigned(message, 2);
            qDebug() << "received change request for generator non-repetition interval;"
                     << "seconds:" << intervalSeconds;

            if (intervalSeconds < 0) {
                return; /* invalid message */
            }

            _serverInterface->setTrackRepetitionAvoidanceSeconds(intervalSeconds);
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
            if (entry == nullptr) {
                return; /* not found :-/ */
            }

            const FileHash* hash = entry->hash();
            if (hash == nullptr) {
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

            if (!isLoggedIn()) return; /* client needs to be authenticated for this */

            quint32 queueID = NetworkUtil::get4Bytes(message, 2);
            qint64 position = NetworkUtil::get8BytesSigned(message, 6);

            qDebug() << "received seek command; QID:" << queueID
                     << "  position:" << position;

            if (queueID != _player->nowPlayingQID()) {
                return; /* invalid queue ID */
            }

            if (position < 0) return; /* invalid position */

            _serverInterface->seekTo(position);
        }
            break;
        case NetworkProtocol::QueueEntryMoveRequestMessage:
        {
            if (messageLength != 8) {
                return; /* invalid message */
            }

            if (!isLoggedIn()) return; /* client needs to be authenticated for this */

            qint16 move = NetworkUtil::get2BytesSigned(message, 2);
            quint32 queueID = NetworkUtil::get4Bytes(message, 4);

            qDebug() << "received track move command; QID:" << queueID
                     << " move:" << move;

            _serverInterface->moveQueueEntry(queueID, move);
        }
            break;
        case NetworkProtocol::InitiateNewUserAccountMessage:
        {
            if (messageLength <= 8) {
                return; /* invalid message */
            }

            int loginLength = NetworkUtil::getByteUnsignedToInt(message, 2);
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

            int loginLength = NetworkUtil::getByteUnsignedToInt(message, 2);
            int saltLength = NetworkUtil::getByteUnsignedToInt(message, 3);
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

            int loginLength = NetworkUtil::getByteUnsignedToInt(message, 2);
            quint32 clientReference = NetworkUtil::get4Bytes(message, 4);

            if (messageLength - 8 != loginLength) {
                return; /* invalid message */
            }

            qDebug() << "received initiate-login request; clientRef:"
                     << clientReference;

            if (isLoggedIn()) { /* already logged in */
                sendResultMessage(NetworkProtocol::AlreadyLoggedIn, clientReference, 0);
                return;
            }

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

            int loginLength = NetworkUtil::getByteUnsignedToInt(message, 4);
            int userSaltLength = NetworkUtil::getByteUnsignedToInt(message, 5);
            int sessionSaltLength = NetworkUtil::getByteUnsignedToInt(message, 6);
            int hashedPasswordLength = NetworkUtil::getByteUnsignedToInt(message, 7);
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
                _serverInterface->setLoggedIn(user.id, user.login);
                sendSuccessMessage(clientReference, user.id);
            }
            else {
                sendResultMessage(
                    NetworkProtocol::UserLoginAuthenticationFailed, clientReference, 0
                );
            }
        }
            break;
        case NetworkProtocol::CollectionFetchRequestMessage:
        {
            if (messageLength != 8) {
                return; /* invalid message */
            }

            quint32 clientReference = NetworkUtil::get4Bytes(message, 4);

            if (!isLoggedIn()) {
                /* client needs to be authenticated for this */
                sendResultMessage(NetworkProtocol::NotLoggedIn, clientReference, 0);
                return;
            }

            handleCollectionFetchRequest(clientReference);
        }
            break;
        case NetworkProtocol::AddHashToEndOfQueueRequestMessage:
        case NetworkProtocol::AddHashToFrontOfQueueRequestMessage:
            parseAddHashToQueueRequest(message, messageType);
            break;
        case NetworkProtocol::HashUserDataRequestMessage:
            parseHashUserDataRequest(message);
            break;
        case NetworkProtocol::InsertHashIntoQueueRequestMessage:
            parseInsertHashIntoQueueRequest(message);
            break;
        case NetworkProtocol::PlayerHistoryRequestMessage:
            parsePlayerHistoryRequest(message);
            break;
        case NetworkProtocol::QueueEntryDuplicationRequestMessage:
            parseQueueEntryDuplicationRequest(message);
            break;
        case NetworkProtocol::CompatibilityInterfaceDefinitionsRequest:
            parseCompatibilityInterfaceDefinitionsRequest(message);
            break;
        case NetworkProtocol::CompatibilityInterfaceTriggerActionRequest:
            parseCompatibilityInterfaceTriggerActionRequest(message);
            break;
        default:
            qDebug() << "received unknown binary message type" << messageType
                     << " with length" << messageLength;
            break; /* unknown message type */
        }
    }

    void ConnectedClient::handleExtensionMessage(quint8 extensionId, quint8 messageType,
                                                 QByteArray const& message)
    {
        /* parse extensions here */

        //if (extensionId == _knownExtensionOther.id) {
        //    switch (messageType) {
        //    case 1: parseExtensionMessage1(message); break;
        //    case 2: parseExtensionMessage2(message); break;
        //    case 3: parseExtensionMessage3(message); break;
        //    }
        //}

        qWarning() << "unhandled extension message" << messageType
                   << "for extension" << extensionId
                   << "with length" << message.length()
                   << "; extension name: "
                   << _clientExtensionNames.value(extensionId, "?");
    }

    void ConnectedClient::parseClientProtocolExtensionsMessage(QByteArray const& message)
    {
        if (message.length() < 4) {
            return; /* invalid message */
        }

        /* be strict about reserved space */
        int filler = NetworkUtil::getByteUnsignedToInt(message, 2);
        if (filler != 0) {
            return; /* invalid message */
        }

        int extensionCount = NetworkUtil::getByteUnsignedToInt(message, 3);
        if (message.length() < 4 + extensionCount * 4) {
            return; /* invalid message */
        }

        QVector<NetworkProtocol::ProtocolExtension> extensions;
        QSet<quint8> ids;
        QSet<QString> names;
        extensions.reserve(extensionCount);
        ids.reserve(extensionCount);
        names.reserve(extensionCount);

        int offset = 4;
        for (int i = 0; i < extensionCount; ++i) {
            if (offset > message.length() - 3) {
                return; /* invalid message */
            }

            quint8 id = NetworkUtil::getByte(message, offset);
            quint8 version = NetworkUtil::getByte(message, offset + 1);
            quint8 byteCount = NetworkUtil::getByte(message, offset + 2);
            offset += 3;

            if (id == 0 || version == 0 || byteCount == 0
                    || offset > message.length() - byteCount)
            {
                return; /* invalid message */
            }

            QString name = NetworkUtil::getUtf8String(message, offset, byteCount);
            offset += byteCount;

            if (ids.contains(id) || names.contains(name)) {
                return; /* invalid message */
            }

            ids << id;
            names << name;
            extensions << NetworkProtocol::ProtocolExtension(id, name, version);
        }

        if (offset != message.length()) {
            return; /* invalid message */
        }

        registerClientProtocolExtensions(extensions);
    }

    void ConnectedClient::parseAddHashToQueueRequest(const QByteArray& message,
                                           NetworkProtocol::ClientMessageType messageType)
    {
        qDebug() << "received 'add filehash to queue' request";

        if (message.length() != 2 + 2 + NetworkProtocol::FILEHASH_BYTECOUNT) {
            /* invalid message */
            return;
        }

        if (!isLoggedIn()) { return; /* client needs to be authenticated for this */ }

        bool ok;
        FileHash hash = NetworkProtocol::getHash(message, 4, &ok);
        if (!ok || hash.isNull()) {
            return; /* invalid message */
        }

        qDebug() << " request contains hash:" << hash.dumpToString();

        if (messageType == NetworkProtocol::AddHashToEndOfQueueRequestMessage) {
            _serverInterface->enqueue(hash);
        }
        else if (messageType == NetworkProtocol::AddHashToFrontOfQueueRequestMessage) {
            _serverInterface->insertAtFront(hash);
        }
        else {
            return; /* invalid message */
        }
    }

    void ConnectedClient::parseInsertHashIntoQueueRequest(const QByteArray &message) {
        qDebug() << "received 'insert filehash into queue at index' request";

        if (message.length() != 2 + 2 + 4 + 4 + NetworkProtocol::FILEHASH_BYTECOUNT) {
            return; /* invalid message */
        }

        if (!isLoggedIn()) { return; /* client needs to be authenticated for this */ }

        quint32 clientReference = NetworkUtil::get4Bytes(message, 4);
        quint32 index = NetworkUtil::get4Bytes(message, 8);

        qDebug() << " client ref:" << clientReference << "; " << "index:" << index;

        bool ok;
        FileHash hash = NetworkProtocol::getHash(message, 12, &ok);
        if (!ok || hash.isNull()) {
            return; /* invalid message */
        }

        qDebug() << " request contains hash:" << hash.dumpToString();

        PlayerQueue& queue = _player->queue();
        auto entry = new QueueEntry(&queue, hash);

        _trackAdditionConfirmationsPending[entry->queueID()] = clientReference;
        _serverInterface->insertAtIndex(index, entry);
    }

    void ConnectedClient::parseQueueEntryRemovalRequest(QByteArray const& message) {
        if (message.length() != 6) {
            return; /* invalid message */
        }

        if (!isLoggedIn()) return; /* client needs to be authenticated for this */

        quint32 queueID = NetworkUtil::get4Bytes(message, 2);
        qDebug() << "received removal request for QID" << queueID;

        if (queueID <= 0) {
            return; /* invalid queue ID */
        }

        _serverInterface->removeQueueEntry(queueID);
    }

    void ConnectedClient::parseQueueEntryDuplicationRequest(QByteArray const& message) {
        qDebug() << "received 'duplicate queue entry' request";

        if (message.length() != 2 + 2 + 4 + 4) {
            return; /* invalid message */
        }

        if (!isLoggedIn()) return; /* client needs to be authenticated for this */

        quint32 clientReference = NetworkUtil::get4Bytes(message, 4);
        quint32 queueID = NetworkUtil::get4Bytes(message, 8);

        qDebug() << " client ref:" << clientReference << "; " << "QID:" << queueID;

        if (queueID <= 0) {
            sendResultMessage(NetworkProtocol::QueueIdNotFound, clientReference, queueID);
            return; /* invalid queue ID */
        }

        auto& queue = _player->queue();

        auto index = queue.findIndex(queueID);
        if (index < 0)
        {
            sendResultMessage(NetworkProtocol::QueueIdNotFound, clientReference, queueID);
            return; /* not found; */
        }

        auto existing = queue.entryAtIndex(index);
        if (!existing || existing->queueID() != queueID)
        {
            qWarning() << "queue inconsistency for QID" << queueID;
            sendNonFatalInternalErrorResultMessage(clientReference);
            return; /* not found; */
        }

        auto entry = new QueueEntry(&queue, existing);

        _trackAdditionConfirmationsPending[entry->queueID()] = clientReference;
        _serverInterface->insertAtIndex(index + 1, entry);
    }

    void ConnectedClient::parseHashUserDataRequest(const QByteArray& message) {
        if (message.length() <= 2 + 2 + 4) {
            return; /* invalid message */
        }

        if (!isLoggedIn()) { return; /* client needs to be authenticated for this */ }

        quint16 fields = NetworkUtil::get2Bytes(message, 2);
        quint32 userId = NetworkUtil::get4Bytes(message, 4);

        int offset = 2 + 2 + 4;
        int hashCount = (message.length() - offset) / NetworkProtocol::FILEHASH_BYTECOUNT;
        if ((message.length() - offset) % NetworkProtocol::FILEHASH_BYTECOUNT != 0) {
            return; /* invalid message */
        }

        qDebug() << "received request for user track data; user:" << userId
                 << " track count:" << hashCount << " fields:" << fields;

        fields = fields & 3; /* filter non-supported fields */

        if (fields == 0) return; /* no data that we can return */

        QVector<FileHash> hashes;
        hashes.reserve(hashCount);

        for (int i = 0; i < hashCount; ++i) {
            bool ok;
            auto hash = NetworkProtocol::getHash(message, offset, &ok);
            if (!ok || hash.isNull()) continue;

            hashes.append(hash);
            offset += NetworkProtocol::FILEHASH_BYTECOUNT;
        }

        auto fetcher =
            new UserDataForHashesFetcher(
                userId, hashes,
                (fields & 1) == 1, (fields & 2) == 2,
                _player->resolver()
            );
        connect(
            fetcher, &UserDataForHashesFetcher::finishedWithResult,
            this, &ConnectedClient::userDataForHashesFetchCompleted
        );
        QThreadPool::globalInstance()->start(fetcher);
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

    void ConnectedClient::parseCompatibilityInterfaceDefinitionsRequest(
                                                                const QByteArray& message)
    {
        qDebug() << "received compatibility interface definitions request";

        if (message.size() < 14)
            return; /* invalid message */

        int interfaceCount = NetworkUtil::get2BytesUnsignedToInt(message, 2);
        auto primaryLanguageEncoded = message.mid(5, 3);
        auto alternativeLanguageEncoded = message.mid(9, 3);

        auto primaryLanguage = NetworkProtocol::decodeLanguage(primaryLanguageEncoded);
        auto alternativeLanguage =
                NetworkProtocol::decodeLanguage(alternativeLanguageEncoded);

        qDebug() << "compatibility interface definition request has primary language"
                 << primaryLanguage << "and alternative language" << alternativeLanguage;

        int countWithPaddingIncluded =
                (interfaceCount % 2 == 1) ? (interfaceCount + 1) : interfaceCount;

        if (message.size() != 12 + 2 * countWithPaddingIncluded)
            return; /* invalid message */

        QVector<int> ids;
        ids.reserve(interfaceCount);

        int offset = 12;
        for (int i = 0; i < interfaceCount; ++i)
        {
            int interfaceId = NetworkUtil::get2BytesUnsignedToInt(message, offset);
            offset += 2;

            ids.append(interfaceId);
        }

        auto language =
                CompatibilityUiController::getSupportedLanguage(primaryLanguage,
                                                                alternativeLanguage);

        qDebug() << "compatibility interface definitions will be sent with language:"
                 << language;

        for (int id : ids)
        {
            QTimer::singleShot(
                0, this,
                [this, id, language]()
                {
                    sendCompatibilityInterfaceDefinition(id, language);
                }
            );
        }
    }

    void ConnectedClient::parseCompatibilityInterfaceTriggerActionRequest(
                                                                const QByteArray& message)
    {
        qDebug() << "received compatibility interface action trigger request";

        if (message.length() != 12)
            return; /* invalid message */

        int interfaceId = NetworkUtil::get2BytesUnsignedToInt(message, 4);
        int actionId = NetworkUtil::get2BytesUnsignedToInt(message, 6);
        quint32 clientReference = NetworkUtil::get4Bytes(message, 8);

        qDebug() << "action trigger request is for interface" << interfaceId
                 << "and action" << actionId << "; client reference:" << clientReference;

        // TODO
    }

    void ConnectedClient::handleSingleByteAction(quint8 action)
    {
        /* actions 100-200 represent a SET VOLUME command */
        if (action >= 100 && action <= 200) {
            qDebug() << "received CHANGE VOLUME command, volume"
                     << (uint(action - 100));

            _serverInterface->setVolume(action - 100);
            return;
        }

        /* Other actions */

        switch (action) {
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
            _serverInterface->insertBreakAtFront();
            break;
        case 10: /* request for state info */
            qDebug() << "received request for player status";
            sendStateInfo();
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
            sendServerNameMessage(0, QHostInfo::localHostName());
            break;
        case 17:
            qDebug() << "received request for database UUID";
            sendDatabaseIdentifier();
            break;
        case 18:
            qDebug() << "received request for list of protocol extensions";
            sendProtocolExtensionsMessage();
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
        case 52:
            qDebug() << "received SUBSCRIBE TO COMPATIBILITY INTERFACES command";
            enableCompatibilityInterfaceEvents(GeneralOrSpecific::Specific);
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

    void ConnectedClient::handleCollectionFetchRequest(uint clientReference) {
        auto sender =
            new CollectionSender(this, clientReference, &_player->resolver());

        connect(sender, &CollectionSender::sendCollectionList,
                this, &ConnectedClient::onCollectionTrackInfoBatchToSend);

        connect(sender, &CollectionSender::allSent,
                this, &ConnectedClient::onCollectionTrackInfoCompleted);
    }

    void ConnectedClient::registerClientProtocolExtensions(
                            const QVector<NetworkProtocol::ProtocolExtension>& extensions)
    {
        /* handle extensions here */
        for (auto const& extension : extensions)
        {
            qDebug() << "client will use ID" << extension.id
                     << "and version" << extension.version
                     << "for protocol extension" << extension.name;

            _clientExtensionNames[extension.id] = extension.name;

            //if (extension.name == "known_extension_name") {
            //    _knownExtensionOther = extension;
            //}
        }
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

    void CollectionSender::sendNextBatch() {
        if (_currentIndex >= _hashes.size()) {
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
        if (!infoToSend.isEmpty()) {
            Q_EMIT sendCollectionList(_clientRef, infoToSend);
        }
    }

}
