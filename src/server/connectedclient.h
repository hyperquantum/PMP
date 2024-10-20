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

#ifndef PMP_SERVER_CONNECTEDCLIENT_H
#define PMP_SERVER_CONNECTEDCLIENT_H

#include "common/filehash.h"
#include "common/networkprotocol.h"
#include "common/networkprotocolextensions.h"
#include "common/scrobblerstatus.h"
#include "common/scrobblingprovider.h"
#include "common/startstopeventstatus.h"

#include "collectiontrackinfo.h"
#include "hashstats.h"
#include "historyentry.h"
#include "recenthistoryentry.h"
#include "result.h"
#include "serverplayerstate.h"

#include <QByteArray>
#include <QDateTime>
#include <QHash>
#include <QList>
#include <QSharedPointer>
#include <QTcpSocket>
#include <QVector>

namespace PMP::Server
{
    class CollectionMonitor;
    class CollectionSender;
    class Player;
    class QueueEntry;
    class Resolver;
    class Scrobbling;
    class ServerInterface;
    class ServerHealthMonitor;
    class Users;

    class ConnectedClient : public QObject
    {
        Q_OBJECT
    public:
        ConnectedClient(QTcpSocket* socket, ServerInterface* serverInterface,
                        Player* player, Users* users,
                        CollectionMonitor* collectionMonitor,
                        ServerHealthMonitor* serverHealthMonitor, Scrobbling* scrobbling);

        ~ConnectedClient();

    private Q_SLOTS:
        void terminateConnection();
        void dataArrived();
        void socketError(QAbstractSocket::SocketError error);

        void serverHealthChanged(bool databaseUnavailable, bool sslLibrariesMissing);

        void serverSettingsReloadResultEvent(uint clientReference,
                                             ResultMessageErrorCode errorCode);

        void onFullIndexationStatusEvent(StartStopEventStatus status);
        void onQuickScanForNewFilesStatusEvent(StartStopEventStatus status);

        void volumeChanged(int volume);
        void onDynamicModeStatusEvent(StartStopEventStatus dynamicModeStatus,
                                      int noRepetitionSpanSeconds);
        void onDynamicModeWaveStatusEvent(StartStopEventStatus waveStatus,
                                          quint32 user,
                                          int waveDeliveredCount,
                                          int waveTotalCount);
        void playerStateChanged(ServerPlayerState state);
        void currentTrackChanged(QSharedPointer<QueueEntry const> entry);
        void newHistoryEntry(QSharedPointer<RecentHistoryEntry> entry);
        void trackPositionChanged(qint64 position);
        void onDelayedStartActiveChanged();
        void sendPlayerStateMessage();
        void sendStateInfoAfterTimeout();
        void sendVolumeMessage();
        void sendDynamicModeStatusMessage(StartStopEventStatus enabledStatus,
                                          int noRepetitionSpanSeconds);
        void sendUserPlayingForModeMessage();
        void sendTextualQueueInfo();
        void queueEntryRemoved(qint32 offset, quint32 queueID);
        void queueEntryAddedWithoutReference(qint32 index, quint32 queueId);
        void queueEntryAddedWithReference(qint32 index, quint32 queueId,
                                          quint32 clientReference);
        void queueEntryMoved(quint32 fromOffset, quint32 toOffset, quint32 queueID);
        void onUserPlayingForChanged(quint32 user);
        void onCollectionTrackInfoBatchToSend(uint clientReference,
                                              QVector<CollectionTrackInfo> tracks);
        void onCollectionTrackInfoCompleted(uint clientReference);
        void onHashAvailabilityChanged(QVector<PMP::FileHash> available,
                                       QVector<PMP::FileHash> unavailable);
        void onHashInfoChanged(QVector<CollectionTrackInfo> changes);

        void onScrobblingProviderInfo(ScrobblingProvider provider, ScrobblerStatus status,
                                      bool enabled);
        void onScrobblerStatusChanged(ScrobblingProvider provider,
                                      ScrobblerStatus status);
        void onScrobblingProviderEnabledChanged(ScrobblingProvider provider,
                                                bool enabled);

    private:
        enum class GeneralOrSpecific { General, Specific };

        void enableEvents();
        void enableHealthEvents(GeneralOrSpecific howEnabled);

        bool isLoggedIn() const;
        void connectSlotsAfterSuccessfulUserLogin(quint32 userLoggedIn);

        void readTextCommands();
        void readBinaryCommands();
        void executeTextCommand(QString const& commandText);
        void executeTextCommandWithoutArgs(QString const& command);
        void executeTextCommandWithArgs(QString const& command, QString const& arg1);
        void executeTextCommandWithArgs(QString const& command, QString const& arg1,
                                        QString const& arg2);
        void sendTextCommand(QString const& command);
        void handleBinaryModeSwitchRequest();
        void appendScrobblingMessageStart(QByteArray& buffer,
                                          ScrobblingServerMessageType messageType);
        void sendBinaryMessage(QByteArray const& message);
        void sendKeepAliveReply(quint8 blob);
        void sendProtocolExtensionsMessage();
        void sendEventNotificationMessage(ServerEventCode eventCode);
        void sendServerVersionInfoMessage();
        void sendServerInstanceIdentifier();
        void sendDatabaseIdentifier();
        void sendUsersList();
        void sendGeneratorWaveStatusMessage(StartStopEventStatus status,
                                            quint32 user,
                                            int waveDeliveredCount,
                                            int waveTotalCount);
        void sendQueueContentMessage(qint32 startOffset, quint8 length);
        void sendQueueEntryRemovedMessage(qint32 offset, quint32 queueID);
        void sendQueueEntryAddedMessage(qint32 offset, quint32 queueID);
        void sendQueueEntryAdditionConfirmationMessage(quint32 clientReference,
                                                       qint32 index, quint32 queueID);
        void sendQueueEntryMovedMessage(qint32 fromOffset, qint32 toOffset,
                                        quint32 queueID);
        void sendQueueEntryInfoMessage(quint32 queueID);
        void sendQueueEntryInfoMessage(QList<quint32> const& queueIDs);
        void sendQueueEntryHashMessage(QList<quint32> const& queueIDs);
        quint16 createTrackStatusFor(QSharedPointer<QueueEntry> entry);
        void sendPossibleTrackFilenames(quint32 queueID, QVector<QString> const& names);
        void sendNewUserAccountSaltMessage(QString login, QByteArray const& salt);
        void sendSuccessMessage(quint32 clientReference, quint32 intData);
        void sendSuccessMessage(quint32 clientReference, quint32 intData,
                                QByteArray const& blobData);
        void sendScrobblingResultMessage(ScrobblingResultMessageCode code,
                                         quint32 clientReference);
        void sendResultMessage(Result const& result, quint32 clientReference);
        void sendResultMessage(ResultMessageErrorCode errorType, quint32 clientReference);
        void sendResultMessage(ResultMessageErrorCode errorType, quint32 clientReference,
                               quint32 intData);
        void sendResultMessage(ResultMessageErrorCode errorType, quint32 clientReference,
                               quint32 intData, QByteArray const& blobData);
        void sendExtensionResultMessage(quint8 extensionId, quint8 resultCode,
                                        quint32 clientReference);
        void sendExtensionResultMessageAsRegularResultMessage(quint8 extensionId,
                                                              quint8 resultCode,
                                                              quint32 clientReference);
        void sendNonFatalInternalErrorResultMessage(quint32 clientReference);
        void sendUserLoginSaltMessage(QString login, QByteArray const& userSalt,
                                      QByteArray const& sessionSalt);
        void sendTrackAvailabilityBatchMessage(QVector<FileHash> available,
                                               QVector<FileHash> unavailable);
        void sendTrackInfoBatchMessage(uint clientReference, bool isNotification,
                                       QVector<CollectionTrackInfo> tracks);
        void sendNewHistoryEntryMessage(QSharedPointer<RecentHistoryEntry> entry);
        void sendQueueHistoryMessage(int limit);
        void sendHistoryFragmentMessage(uint clientReference, HistoryFragment fragment);
        void sendHashUserDataMessage(quint32 userId, QVector<HashStats> stats);
        void sendHashInfoReply(uint clientReference, CollectionTrackInfo info);
        void sendServerNameMessage();
        void sendServerHealthMessageIfNotEverythingOkay();
        void sendServerHealthMessage();
        void sendServerClockMessage();
        void sendDelayedStartInfoMessage();

        void sendScrobblingProviderInfoMessage(quint32 userId,
                                               ScrobblingProvider provider,
                                               ScrobblerStatus status, bool enabled);
        void sendScrobblerStatusChangedMessage(quint32 userId,
                                               ScrobblingProvider provider,
                                               ScrobblerStatus newStatus);
        void sendScrobblingProviderEnabledChangeMessage(quint32 userId,
                                                        ScrobblingProvider provider,
                                                        bool enabled);

        void sendIndexationStatusMessage(StartStopEventStatus fullIndexationStatus,
                                         StartStopEventStatus quickScanForNewFilesStatus);

        void handleBinaryMessage(QByteArray const& message);
        void handleStandardBinaryMessage(ClientMessageType messageType,
                                         QByteArray const& message);
        void handleExtensionMessage(quint8 extensionId, quint8 messageType,
                                    QByteArray const& message);
        void handleSingleByteAction(quint8 action);
        void handleParameterlessAction(ParameterlessActionCode code,
                                       quint32 clientReference);
        void handleCollectionFetchRequest(uint clientReference);

        void parseKeepAliveMessage(QByteArray const& message);
        void parseClientProtocolExtensionsMessage(QByteArray const& message);
        void parseSingleByteActionMessage(QByteArray const& message);
        void parseParameterlessActionMessage(QByteArray const& message);
        void parseInitiateNewUserAccountMessage(QByteArray const& message);
        void parseFinishNewUserAccountMessage(QByteArray const& message);
        void parseInitiateLoginMessage(QByteArray const& message);
        void parseFinishLoginMessage(QByteArray const& message);
        void parseActivateDelayedStartRequest(QByteArray const& message);
        void parsePlayerSeekRequestMessage(QByteArray const& message);
        void parseTrackInfoRequestMessage(QByteArray const& message);
        void parseBulkTrackInfoRequestMessage(QByteArray const& message);
        void parseBulkQueueEntryHashRequestMessage(QByteArray const& message);
        void parsePossibleFilenamesForQueueEntryRequestMessage(QByteArray const& message);
        void parseQueueFetchRequestMessage(QByteArray const& message);
        void parseAddHashToQueueRequest(QByteArray const& message,
                                        ClientMessageType messageType);
        void parseInsertSpecialQueueItemRequest(QByteArray const& message);
        void parseInsertHashIntoQueueRequest(QByteArray const& message);
        void parseQueueEntryRemovalRequest(QByteArray const& message);
        void parseQueueEntryDuplicationRequest(QByteArray const& message);
        void parseQueueEntryMoveRequestMessage(QByteArray const& message);
        void parseHashUserDataRequest(QByteArray const& message);
        void parseHashInfoRequest(QByteArray const& message);
        void parsePersonalHistoryRequest(QByteArray const& message);
        void parsePlayerHistoryRequest(QByteArray const& message);
        void parseCurrentUserScrobblingProviderInfoRequestMessage(
                                                               QByteArray const& message);
        void parseUserScrobblingEnableDisableRequest(QByteArray const& message);
        void parseScrobblingAuthenticationRequestMessage(QByteArray const& message);
        void parseGeneratorNonRepetitionChangeMessage(QByteArray const& message);
        void parseCollectionFetchRequestMessage(QByteArray const& message);

        void schedulePlayerStateNotification();

        static const qint16 ServerProtocolNo;

        QTcpSocket* _socket;
        ServerInterface* _serverInterface;
        Player* _player;
        Users* _users;
        CollectionMonitor* _collectionMonitor;
        ServerHealthMonitor* _serverHealthMonitor;
        Scrobbling* _scrobbling;
        QByteArray _textReadBuffer;
        int _clientProtocolNo;
        NetworkProtocolExtensionSupportMap _extensionsThis;
        NetworkProtocolExtensionSupportMap _extensionsOther;
        quint32 _lastSentNowPlayingID;
        QString _userAccountRegistering;
        QByteArray _saltForUserAccountRegistering;
        quint32 _userIdLoggingIn { 0 };
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
