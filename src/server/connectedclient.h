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

#ifndef PMP_CONNECTEDCLIENT_H
#define PMP_CONNECTEDCLIENT_H

#include "common/collectiontrackinfo.h"
#include "common/filehash.h"
#include "common/networkprotocol.h"

#include "playerhistoryentry.h"
#include "result.h"
#include "serverplayerstate.h"
#include "userdataforhashesfetcher.h"

#include <QByteArray>
#include <QDateTime>
#include <QHash>
#include <QList>
#include <QSharedPointer>
#include <QTcpSocket>
#include <QVector>

namespace PMP
{
    class CollectionMonitor;
    class CollectionSender;
    class History;
    class Player;
    class QueueEntry;
    class Resolver;
    class Server;
    class ServerInterface;
    class ServerHealthMonitor;
    class Users;

    class ConnectedClient : public QObject
    {
        Q_OBJECT
    public:
        ConnectedClient(QTcpSocket* socket, ServerInterface* serverInterface,
                        Player* player, History* history, Users* users,
                        CollectionMonitor* collectionMonitor,
                        ServerHealthMonitor* serverHealthMonitor);

        ~ConnectedClient();

    private Q_SLOTS:

        void terminateConnection();
        void dataArrived();
        void socketError(QAbstractSocket::SocketError error);

        void serverHealthChanged(bool databaseUnavailable);

        void serverSettingsReloadResultEvent(uint clientReference,
                                             ResultMessageErrorCode errorCode);

        void volumeChanged(int volume);
        void onDynamicModeStatusEvent(StartStopEventStatus dynamicModeStatus,
                                      int noRepetitionSpanSeconds);
        void onDynamicModeWaveStatusEvent(StartStopEventStatus waveStatus,
                                          quint32 user,
                                          int waveDeliveredCount,
                                          int waveTotalCount);
        void playerStateChanged(ServerPlayerState state);
        void currentTrackChanged(QueueEntry const* entry);
        void newHistoryEntry(QSharedPointer<PlayerHistoryEntry> entry);
        void trackPositionChanged(qint64 position);
        void sendPlayerStateMessage();
        void sendStateInfoAfterTimeout();
        void sendVolumeMessage();
        void sendDynamicModeStatusMessage(StartStopEventStatus enabledStatus,
                                          int noRepetitionSpanSeconds);
        void sendUserPlayingForModeMessage();
        void sendTextualQueueInfo();
        void queueEntryRemoved(quint32 offset, quint32 queueID);
        void queueEntryAddedWithoutReference(quint32 index, quint32 queueId);
        void queueEntryAddedWithReference(quint32 index, quint32 queueId,
                                          quint32 clientReference);
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
        void sendProtocolExtensionsMessage();
        void sendEventNotificationMessage(ServerEventCode eventCode);
        void sendServerInstanceIdentifier();
        void sendDatabaseIdentifier();
        void sendUsersList();
        void sendGeneratorWaveStatusMessage(StartStopEventStatus status,
                                            quint32 user,
                                            int waveDeliveredCount,
                                            int waveTotalCount);
        void sendQueueContentMessage(qint32 startOffset, quint8 length);
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
        void sendResultMessage(Result const& result, quint32 clientReference);
        void sendResultMessage(ResultMessageErrorCode errorType, quint32 clientReference,
                               quint32 intData);
        void sendResultMessage(ResultMessageErrorCode errorType, quint32 clientReference,
                               quint32 intData, QByteArray const& blobData);
        void sendNonFatalInternalErrorResultMessage(quint32 clientReference);
        void sendUserLoginSaltMessage(QString login, QByteArray const& userSalt,
                                      QByteArray const& sessionSalt);
        void sendTrackAvailabilityBatchMessage(QVector<FileHash> available,
                                               QVector<FileHash> unavailable);
        void sendTrackInfoBatchMessage(uint clientReference, bool isNotification,
                                       QVector<CollectionTrackInfo> tracks);
        void sendNewHistoryEntryMessage(QSharedPointer<PlayerHistoryEntry> entry);
        void sendQueueHistoryMessage(int limit);
        void sendServerNameMessage();
        void sendServerHealthMessageIfNotEverythingOkay();
        void sendServerHealthMessage();
        void sendServerClockMessage();

        void handleBinaryMessage(QByteArray const& message);
        void handleStandardBinaryMessage(ClientMessageType messageType,
                                         QByteArray const& message);
        void handleExtensionMessage(quint8 extensionId, quint8 messageType,
                                    QByteArray const& message);
        void registerClientProtocolExtensions(
                           const QVector<NetworkProtocol::ProtocolExtension>& extensions);
        void handleSingleByteAction(quint8 action);
        void handleParameterlessAction(ParameterlessActionCode code,
                                       quint32 clientReference);
        void handleCollectionFetchRequest(uint clientReference);

        void parseClientProtocolExtensionsMessage(QByteArray const& message);
        void parseSingleByteActionMessage(QByteArray const& message);
        void parseParameterlessActionMessage(QByteArray const& message);
        void parseTrackInfoRequestMessage(QByteArray const& message);
        void parseBulkTrackInfoRequestMessage(QByteArray const& message);
        void parseBulkQueueEntryHashRequestMessage(QByteArray const& message);
        void parseQueueFetchRequestMessage(QByteArray const& message);
        void parseAddHashToQueueRequest(QByteArray const& message,
                                        ClientMessageType messageType);
        void parseInsertSpecialQueueItemRequest(QByteArray const& message);
        void parseInsertHashIntoQueueRequest(QByteArray const& message);
        void parseQueueEntryRemovalRequest(QByteArray const& message);
        void parseQueueEntryDuplicationRequest(QByteArray const& message);
        void parseHashUserDataRequest(QByteArray const& message);
        void parsePlayerHistoryRequest(QByteArray const& message);

        void schedulePlayerStateNotification();

        static const qint16 ServerProtocolNo;

        QTcpSocket* _socket;
        ServerInterface* _serverInterface;
        Player* _player;
        History* _history;
        Users* _users;
        CollectionMonitor* _collectionMonitor;
        ServerHealthMonitor* _serverHealthMonitor;
        QByteArray _textReadBuffer;
        int _clientProtocolNo;
        QHash<quint8, QString> _clientExtensionNames;
        quint32 _lastSentNowPlayingID;
        QString _userAccountRegistering;
        QByteArray _saltForUserAccountRegistering;
        QString _userAccountLoggingIn;
        QByteArray _sessionSaltForUserLoggingIn;
        bool _terminated;
        bool _binaryMode;
        bool _eventsEnabled;
        bool _healthEventsEnabled;
        bool _pendingPlayerStatus;
    };

    class CollectionSender : public QObject
    {
        Q_OBJECT
    public:
        CollectionSender(ConnectedClient* connection, uint clientReference,
                         Resolver* resolver);

    Q_SIGNALS:
        void sendCollectionList(uint clientReference,
                                QVector<CollectionTrackInfo> tracks);
        void allSent(uint clientReference);

    private Q_SLOTS:
        void sendNextBatch();

    private:
        uint _clientRef;
        Resolver* _resolver;
        QVector<FileHash> _hashes;
        int _currentIndex;
    };

}
#endif
