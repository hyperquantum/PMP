/*
    Copyright (C) 2014-2022, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_SERVERCONNECTION_H
#define PMP_SERVERCONNECTION_H

#include "common/collectiontrackinfo.h"
#include "common/disconnectreason.h"
#include "common/future.h"
#include "common/networkprotocol.h"
#include "common/playerhistorytrackinfo.h"
#include "common/playerstate.h"
#include "common/queueindextype.h"
#include "common/requestid.h"
#include "common/scrobblingprovider.h"
#include "common/serverhealthstatus.h"
#include "common/specialqueueitemtype.h"
#include "common/tribool.h"
#include "common/userloginerror.h"
#include "common/userregistrationerror.h"
#include "common/versioninfo.h"

#include <QByteArray>
#include <QDateTime>
#include <QElapsedTimer>
#include <QHash>
#include <QList>
#include <QObject>
#include <QTcpSocket>
#include <QUuid>
#include <QVector>

QT_FORWARD_DECLARE_CLASS(QTimer)

namespace PMP::Client
{
    class CollectionFetcher;
    class ServerCapabilities;
    class ServerCapabilitiesImpl;

    enum class ServerEventSubscription
    {
        None = 0,
        AllEvents = 1,
        ServerHealthMessages = 2,
    };

    /**
        Represents a connection to a PMP server.
    */
    class ServerConnection : public QObject
    {
        Q_OBJECT
    private:
        enum State
        {
            NotConnected, Connecting, Handshake, TextMode,
            HandshakeFailure, BinaryHandshake, BinaryMode,
            Aborting, Disconnecting
        };

        class ResultMessageData;
        class ResultHandler;
        class PromiseResultHandler;
        class ParameterlessActionResultHandler;
        class CollectionFetchResultHandler;
        class TrackInsertionResultHandler;
        class QueueEntryInsertionResultHandler;
        class DuplicationResultHandler;

    public:
        explicit ServerConnection(QObject* parent = nullptr,
                                  ServerEventSubscription eventSubscription =
                                                      ServerEventSubscription::AllEvents);
        ~ServerConnection();

        void connectToHost(QString const& host, quint16 port);
        void disconnect();

        ServerCapabilities const& serverCapabilities() const;
        ServerHealthStatus serverHealth() const { return _serverHealthStatus; }

        bool isConnected() const;
        bool isLoggedIn() const { return userLoggedInId() > 0; }

        quint32 userLoggedInId() const;
        QString userLoggedInName() const;

        TriBool doingFullIndexation() const { return _doingFullIndexation; }

        void fetchCollection(CollectionFetcher* fetcher);

        SimpleFuture<ResultMessageErrorCode> reloadServerSettings();
        SimpleFuture<ResultMessageErrorCode> activateDelayedStart(
                                                                qint64 delayMilliseconds);
        SimpleFuture<ResultMessageErrorCode> deactivateDelayedStart();
        RequestID insertQueueEntryAtIndex(FileHash const& hash, quint32 index);
        RequestID insertSpecialQueueItemAtIndex(SpecialQueueItemType itemType, int index,
                                       QueueIndexType indexType = QueueIndexType::Normal);
        RequestID duplicateQueueEntry(uint queueID);

    public Q_SLOTS:
        void shutdownServer();

        void sendDatabaseIdentifierRequest();
        void sendServerInstanceIdentifierRequest();
        void sendServerNameRequest();
        void sendVersionInfoRequest();
        void sendDelayedStartInfoRequest();

        void requestPlayerState();
        void play();
        void pause();
        void skip();

        void seekTo(uint queueID, qint64 position);

        void insertBreakAtFrontIfNotExists();

        void setVolume(int percentage);

        void enableDynamicMode();
        void disableDynamicMode();
        void expandQueue();
        void trimQueue();
        void requestDynamicModeStatus();
        void setDynamicModeNoRepetitionSpan(int seconds);
        void startDynamicModeWave();
        void terminateDynamicModeWave();

        void sendQueueFetchRequest(uint startOffset, quint8 length = 0);
        void deleteQueueEntry(uint queueID);
        void moveQueueEntry(uint queueID, qint16 offsetDiff);

        void insertQueueEntryAtFront(FileHash const& hash);
        void insertQueueEntryAtEnd(FileHash const& hash);

        void sendQueueEntryInfoRequest(uint queueID);
        void sendQueueEntryInfoRequest(QList<uint> const& queueIDs);

        void sendQueueEntryHashRequest(QList<uint> const& queueIDs);

        void sendHashUserDataRequest(quint32 userId, QList<FileHash> const& hashes);

        void sendPossibleFilenamesRequest(uint queueID);

        void sendPlayerHistoryRequest(int limit);

        void sendUserAccountsFetchRequest();
        void createNewUserAccount(QString login, QString password);
        void login(QString login, QString password);

        void switchToPublicMode();
        void switchToPersonalMode();
        void requestUserPlayingForMode();

        void requestScrobblingProviderInfoForCurrentUser();
        void enableScrobblingForCurrentUser(ScrobblingProvider provider);
        void disableScrobblingForCurrentUser(ScrobblingProvider provider);

        void startFullIndexation();
        void requestFullIndexationRunningStatus();

    Q_SIGNALS:
        void connected();
        void disconnected(DisconnectReason reason);
        void cannotConnect(QAbstractSocket::SocketError error);
        void invalidServer();
        void serverHealthReceived();

        void receivedDatabaseIdentifier(QUuid uuid);
        void receivedServerInstanceIdentifier(QUuid uuid);
        void receivedServerVersionInfo(VersionInfo versionInfo);
        void receivedServerName(quint8 nameType, QString name);
        void receivedClientClockTimeOffset(qint64 clientClockTimeOffsetMs);

        void receivedPlayerState(PlayerState state, quint8 volume, quint32 queueLength,
                                 quint32 nowPlayingQID, quint64 nowPlayingPosition,
                                 bool delayedStartActive);
        void receivedDelayedStartInfo(QDateTime serverClockDeadline,
                                      qint64 timeRemainingMilliseconds);

        void volumeChanged(int percentage);

        void dynamicModeStatusReceived(bool enabled, int noRepetitionSpanSeconds);
        void dynamicModeHighScoreWaveStatusReceived(bool active, bool statusChanged,
                                                    int progress, int progressTotal);

        void receivedPlayerHistoryEntry(PMP::PlayerHistoryTrackInfo track);
        void receivedPlayerHistory(QVector<PMP::PlayerHistoryTrackInfo> tracks);

        void receivedQueueContents(int queueLength, int startOffset,
                                   QList<quint32> queueIDs);
        void queueEntryAdded(qint32 offset, quint32 queueId, RequestID requestId);
        void queueEntryInsertionFailed(ResultMessageErrorCode errorCode,
                                       RequestID requestId);
        void queueEntryRemoved(qint32 offset, quint32 queueId);
        void queueEntryMoved(qint32 fromOffset, qint32 toOffset, quint32 queueId);
        void receivedTrackInfo(quint32 queueId, QueueEntryType type,
                               qint64 lengthMilliseconds, QString title, QString artist);
        void receivedQueueEntryHash(quint32 queueId, QueueEntryType type, FileHash hash);
        void receivedHashUserData(FileHash hash, quint32 userId,
                                  QDateTime previouslyHeard, qint16 scorePermillage);
        void receivedPossibleFilenames(quint32 queueId, QList<QString> names);

        void userAccountsReceived(QList<QPair<uint, QString>> accounts);
        void userAccountCreatedSuccessfully(QString login, quint32 id);
        void userAccountCreationError(QString login, UserRegistrationError errorType);

        void userLoggedInSuccessfully(QString login, quint32 id);
        void userLoginError(QString login, UserLoginError errorType);

        void receivedUserPlayingFor(quint32 userId, QString userLogin);

        void fullIndexationStatusReceived(bool running);
        void fullIndexationStarted();
        void fullIndexationFinished();

        void collectionTracksAvailabilityChanged(QVector<PMP::FileHash> available,
                                                 QVector<PMP::FileHash> unavailable);
        void collectionTracksChanged(QVector<PMP::CollectionTrackInfo> changes);

        void scrobblingProviderInfoReceived(ScrobblingProvider provider,
                                            ScrobblerStatus status, bool enabled);
        void scrobblerStatusChanged(ScrobblingProvider provider, ScrobblerStatus status);
        void scrobblingProviderEnabledChanged(ScrobblingProvider provider, bool enabled);

    private Q_SLOTS:
        void onConnected();
        void onDisconnected();
        void onReadyRead();
        void onSocketError(QAbstractSocket::SocketError error);
        void onKeepAliveTimerTimeout();

    private:
        void breakConnection(DisconnectReason reason);

        uint getNewReference();
        RequestID getNewRequestId();
        RequestID signalRequestError(ResultMessageErrorCode errorCode,
                             void (ServerConnection::*errorSignal)(ResultMessageErrorCode,
                                                                   RequestID));
        RequestID signalServerTooOldError(
                             void (ServerConnection::*errorSignal)(ResultMessageErrorCode,
                                                                   RequestID));
        FutureResult<ResultMessageErrorCode> serverTooOldFutureResult();

        void sendTextCommand(QString const& command);
        void appendScrobblingMessageStart(QByteArray& buffer,
                                    NetworkProtocol::ScrobblingClientMessage messageType);
        void sendBinaryMessage(QByteArray const& message);
        void sendKeepAliveMessage();
        void sendProtocolExtensionsMessage();
        void sendSingleByteAction(quint8 action);
        SimpleFuture<ResultMessageErrorCode> sendParameterlessActionRequest(
                                                            ParameterlessActionCode code);

        void readTextCommands();
        void readBinaryCommands();
        void executeTextCommand(QString const& commandText);
        void handleBinaryMessage(QByteArray const& message);
        void handleStandardBinaryMessage(ServerMessageType messageType,
                                         QByteArray const& message);
        void handleExtensionMessage(quint8 extensionId, quint8 extensionMessageType,
                                    QByteArray const& message);
        void handleResultMessage(quint16 errorCode, quint32 clientReference,
                                 quint32 intData, QByteArray const& blobData);
        void registerServerProtocolExtensions(
                           const QVector<NetworkProtocol::ProtocolExtension>& extensions);
        void handleServerEvent(ServerEventCode eventCode);

        void sendInitiateNewUserAccountMessage(QString login, quint32 clientReference);
        void sendFinishNewUserAccountMessage(QString login, QByteArray salt,
                                             QByteArray hashedPassword,
                                             quint32 clientReference);

        void handleNewUserSalt(QString login, QByteArray salt);
        void handleUserRegistrationResult(ResultMessageErrorCode errorCode,
                                          quint32 intData, QByteArray const& blobData);

        void sendInitiateLoginMessage(QString login, quint32 clientReference);
        void sendFinishLoginMessage(QString login, QByteArray userSalt,
                                    QByteArray sessionSalt, QByteArray hashedPassword,
                                    quint32 clientReference);
        void handleLoginSalt(QString login, QByteArray userSalt, QByteArray sessionSalt);
        void handleUserLoginResult(ResultMessageErrorCode errorCode, quint32 intData,
                                   QByteArray const& blobData);

        void sendScrobblingProviderInfoRequest();
        void sendUserScrobblingEnableDisableRequest(ScrobblingProvider provider,
                                                    bool enable);

        void onFullIndexationRunningStatusReceived(bool running);

        void parseKeepAliveMessage(QByteArray const& message);

        void parseSimpleResultMessage(QByteArray const& message);

        void parseServerProtocolExtensionsMessage(QByteArray const& message);
        void parseServerEventNotificationMessage(QByteArray const& message);
        void parseServerInstanceIdentifierMessage(QByteArray const& message);
        void parseServerVersionInfoMessage(QByteArray const& message);
        void parseServerNameMessage(QByteArray const& message);
        void parseDatabaseIdentifierMessage(QByteArray const& message);
        void parseServerHealthMessage(QByteArray const& message);
        void parseServerClockMessage(QByteArray const& message);

        void parseUsersListMessage(QByteArray const& message);
        void parseNewUserAccountSaltMessage(QByteArray const& message);
        void parseUserLoginSaltMessage(QByteArray const& message);

        void parsePlayerStateMessage(QByteArray const& message);
        void parseDelayedStartInfoMessage(QByteArray const& message);
        void parseVolumeChangedMessage(QByteArray const& message);
        void parseUserPlayingForModeMessage(QByteArray const& message);

        void parseQueueContentsMessage(QByteArray const& message);
        void parseTrackInfoMessage(QByteArray const& message);
        void parseBulkTrackInfoMessage(QByteArray const& message);
        void parsePossibleFilenamesForQueueEntryMessage(QByteArray const& message);
        void parseBulkQueueEntryHashMessage(QByteArray const& message);
        void parseQueueEntryAddedMessage(QByteArray const& message);
        void parseQueueEntryAdditionConfirmationMessage(QByteArray const& message);
        void parseQueueEntryRemovedMessage(QByteArray const& message);
        void parseQueueEntryMovedMessage(QByteArray const& message);

        void parseDynamicModeStatusMessage(QByteArray const& message);
        void parseDynamicModeWaveStatusMessage(QByteArray const& message);

        void parseTrackAvailabilityChangeBatchMessage(QByteArray const& message);
        void parseTrackInfoBatchMessage(QByteArray const& message,
                                        ServerMessageType messageType);

        void parseHashUserDataMessage(QByteArray const& message);
        void parseNewHistoryEntryMessage(QByteArray const& message);
        void parsePlayerHistoryMessage(QByteArray const& message);

        void parseScrobblingProviderInfoMessage(QByteArray const& message);
        void parseScrobblerStatusChangeMessage(QByteArray const& message);
        void parseScrobblingProviderEnabledChangeMessage(QByteArray const& message);

        void sendCollectionFetchRequestMessage(uint clientReference);

        void invalidMessageReceived(QByteArray const& message, QString messageType = "",
                                    QString extraInfo = "");

        void receivedServerClockTime(QDateTime serverClockTime);

        static const quint16 ClientProtocolNo;
        static const int KeepAliveIntervalMs;
        static const int KeepAliveReplyTimeoutMs;

        ServerCapabilitiesImpl* _serverCapabilities;
        DisconnectReason _disconnectReason;
        QElapsedTimer _timeSinceLastMessageReceived;
        QTimer* _keepAliveTimer;
        ServerEventSubscription _autoSubscribeToEventsAfterConnect;
        State _state;
        QTcpSocket _socket;
        QByteArray _readBuffer;
        bool _binarySendingMode;
        int _serverProtocolNo;
        QHash<quint8, QString> _serverExtensionNames;
        const NetworkProtocol::ProtocolExtension _scrobblingSupportThis;
        NetworkProtocol::ProtocolExtensionSupport _scrobblingSupportOther;
        uint _nextRef;
        uint _userAccountRegistrationRef;
        QString _userAccountRegistrationLogin;
        QString _userAccountRegistrationPassword;
        uint _userLoginRef;
        QString _userLoggingIn;
        QString _userLoggingInPassword;
        quint32 _userLoggedInId;
        QString _userLoggedInName;
        TriBool _doingFullIndexation;
        QHash<uint, ResultHandler*> _resultHandlers;
        QHash<uint, CollectionFetcher*> _collectionFetchers;
        ServerHealthStatus _serverHealthStatus;
    };
}
#endif
