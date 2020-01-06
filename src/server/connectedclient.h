/*
    Copyright (C) 2014-2020, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_CONNECTEDCLIENT_H
#define PMP_CONNECTEDCLIENT_H

#include "common/collectiontrackinfo.h"
#include "common/filehash.h"
#include "common/networkprotocol.h"

#include "serverplayerstate.h"
#include "userdataforhashesfetcher.h"

#include <QByteArray>
#include <QDateTime>
#include <QHash>
#include <QList>
#include <QTcpSocket>
#include <QVector>

namespace PMP {

    class CollectionMonitor;
    class CollectionSender;
    class Generator;
    class Player;
    class QueueEntry;
    class Resolver;
    class Server;
    class ServerHealthMonitor;
    class Users;

    class ConnectedClient : public QObject {
        Q_OBJECT
    public:
        ConnectedClient(QTcpSocket* socket, Server* server, Player* player,
                        Generator* generator, Users* users,
                        CollectionMonitor* collectionMonitor,
                        ServerHealthMonitor* serverHealthMonitor);

        ~ConnectedClient();

    Q_SIGNALS:

    private slots:

        void terminateConnection();
        void dataArrived();
        void socketError(QAbstractSocket::SocketError error);

        void serverHealthChanged(bool databaseUnavailable);

        void volumeChanged(int volume);
        void dynamicModeStatusChanged(bool enabled);
        void dynamicModeWaveStarting(quint32 user);
        void dynamicModeWaveFinished(quint32 user);
        void dynamicModeNoRepetitionSpanChanged(int seconds);
        void playerStateChanged(ServerPlayerState state);
        void currentTrackChanged(QueueEntry const* entry);
        void trackHistoryEvent(uint queueID, QDateTime started, QDateTime ended,
                               quint32 userPlayedFor, int permillagePlayed, bool hadError,
                               bool hadSeek);
        void trackPositionChanged(qint64 position);
        void sendStateInfo();
        void sendStateInfoAfterTimeout();
        void sendVolumeMessage();
        void sendDynamicModeStatusMessage();
        void sendUserPlayingForModeMessage();
        void sendTextualQueueInfo();
        void queueEntryRemoved(quint32 offset, quint32 queueID);
        void queueEntryAdded(quint32 offset, quint32 queueID);
        void queueEntryMoved(quint32 fromOffset, quint32 toOffset, quint32 queueID);
        void onUserPlayingForChanged(quint32 user);
        void onUserHashStatsUpdated(uint hashID, quint32 user,
                                    QDateTime previouslyHeard, qint16 score);
        void onFullIndexationRunStatusChanged(bool running);
        void onCollectionTrackInfoBatchToSend(uint clientReference,
                                              QVector<CollectionTrackInfo> tracks);
        void onCollectionTrackInfoCompleted(uint clientReference);
        void onHashAvailabilityChanged(QVector<PMP::FileHash> available,
                                       QVector<PMP::FileHash> unavailable);
        void onHashInfoChanged(QVector<CollectionTrackInfo> changes);

        void userDataForHashesFetchCompleted(quint32 userId,
                                             QVector<PMP::UserDataForHash> results,
                                             bool havePreviouslyHeard, bool haveScore);

    private:
        void enableEvents();
        void enableHealthEvents();

        bool isLoggedIn() const;

        void readTextCommands();
        void readBinaryCommands();
        void executeTextCommand(QString const& commandText);
        void executeTextCommandWithoutArgs(QString const& command);
        void executeTextCommandWithArgs(QString const& command, QString const& arg1);
        void executeTextCommandWithArgs(QString const& command, QString const& arg1,
                                        QString const& arg2);
        void sendTextCommand(QString const& command);
        void handleBinaryModeSwitchRequest();
        void sendBinaryMessage(QByteArray const& message);
        void sendEventNotificationMessage(quint8 event);
        void sendServerInstanceIdentifier();
        void sendDatabaseIdentifier();
        void sendUsersList();
        void sendGeneratorWaveStatusMessage(NetworkProtocol::StartStopEventStatus status,
                                            quint32 user);
        void sendQueueContentMessage(quint32 startOffset, quint8 length);
        void sendQueueEntryRemovedMessage(quint32 offset, quint32 queueID);
        void sendQueueEntryAddedMessage(quint32 offset, quint32 queueID);
        void sendQueueEntryAdditionConfirmationMessage(quint32 clientReference,
                                                       quint32 index, quint32 queueID);
        void sendQueueEntryMovedMessage(quint32 fromOffset, quint32 toOffset,
                                        quint32 queueID);
        void sendQueueEntryInfoMessage(quint32 queueID);
        void sendQueueEntryInfoMessage(QList<quint32> const& queueIDs);
        void sendQueueEntryHashMessage(QList<quint32> const& queueIDs);
        quint16 createTrackStatusFor(QueueEntry* entry);
        void sendPossibleTrackFilenames(quint32 queueID, QList<QString> const& names);
        void sendNewUserAccountSaltMessage(QString login, QByteArray const& salt);
        void sendSuccessMessage(quint32 clientReference, quint32 intData);
        void sendSuccessMessage(quint32 clientReference, quint32 intData,
                                QByteArray const& blobData);
        void sendResultMessage(NetworkProtocol::ErrorType errorType,
                              quint32 clientReference, quint32 intData);
        void sendResultMessage(NetworkProtocol::ErrorType errorType,
                              quint32 clientReference, quint32 intData,
                              QByteArray const& blobData);
        void sendNonFatalInternalErrorResultMessage(quint32 clientReference);
        void sendUserLoginSaltMessage(QString login, QByteArray const& userSalt,
                                      QByteArray const& sessionSalt);
        void sendTrackAvailabilityBatchMessage(QVector<FileHash> available,
                                               QVector<FileHash> unavailable);
        void sendTrackInfoBatchMessage(uint clientReference, bool isNotification,
                                       QVector<CollectionTrackInfo> tracks);
        void sendNewHistoryEntryMessage(uint queueID, QDateTime started, QDateTime ended,
                                        quint32 userPlayedFor, int permillagePlayed,
                                        bool hadError, bool hadSeek);
        void sendQueueHistoryMessage(int limit);
        void sendServerNameMessage(quint8 type, QString name);
        void sendServerHealthMessageIfNotEverythingOkay();
        void sendServerHealthMessage();

        void handleBinaryMessage(QByteArray const& message);
        void handleSingleByteAction(quint8 action);
        void handleCollectionFetchRequest(uint clientReference);
        void parseAddHashToQueueRequest(QByteArray const& message,
                                        NetworkProtocol::ClientMessageType messageType);
        void parseInsertHashIntoQueueRequest(QByteArray const& message);
        void parseQueueEntryRemovalRequest(QByteArray const& message);
        void parseQueueEntryDuplicationRequest(QByteArray const& message);
        void parseHashUserDataRequest(QByteArray const& message);
        void parsePlayerHistoryRequest(QByteArray const& message);

        void schedulePlayerStateNotification();

        static const qint16 ServerProtocolNo;

        QTcpSocket* _socket;
        Server* _server;
        Player* _player;
        Generator* _generator;
        Users* _users;
        CollectionMonitor* _collectionMonitor;
        ServerHealthMonitor* _serverHealthMonitor;
        QByteArray _textReadBuffer;
        int _clientProtocolNo;
        quint32 _lastSentNowPlayingID;
        QString _userAccountRegistering;
        QByteArray _saltForUserAccountRegistering;
        QString _userAccountLoggingIn;
        QByteArray _sessionSaltForUserLoggingIn;
        quint32 _userLoggedIn;
        QString _userLoggedInName;
        QHash<quint32, quint32> _trackAdditionConfirmationsPending;
        bool _terminated;
        bool _binaryMode;
        bool _eventsEnabled;
        bool _healthEventsEnabled;
        bool _pendingPlayerStatus;
    };

    class CollectionSender : public QObject {
        Q_OBJECT
    public:
        CollectionSender(ConnectedClient* connection, uint clientReference,
                         Resolver* resolver);

    Q_SIGNALS:
        void sendCollectionList(uint clientReference,
                                QVector<CollectionTrackInfo> tracks);
        void allSent(uint clientReference);

    private slots:
        void sendNextBatch();

    private:
        uint _clientRef;
        Resolver* _resolver;
        QVector<FileHash> _hashes;
        int _currentIndex;
    };

}
#endif
