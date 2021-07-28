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

#ifndef PMP_SERVERCONNECTION_H
#define PMP_SERVERCONNECTION_H

#include "collectiontrackinfo.h"
#include "compatibilityui.h"
#include "networkprotocol.h"
#include "playerhistorytrackinfo.h"
#include "playerstate.h"
#include "serverhealthstatus.h"
#include "tribool.h"
#include "userloginerror.h"

#include <QByteArray>
#include <QDateTime>
#include <QHash>
#include <QList>
#include <QObject>
#include <QSet>
#include <QTcpSocket>
#include <QUuid>
#include <QVector>

namespace PMP {

    class CollectionFetcher;

    class RequestID {
    public:
        RequestID() : _rawId(0) {}
        explicit RequestID(uint rawId) : _rawId(rawId) {}

        bool isValid() const { return _rawId > 0; }
        uint rawId() const { return _rawId; }

    private:
        uint _rawId;
    };

    inline bool operator==(const RequestID& me, const RequestID& other) {
        return me.rawId() == other.rawId();
    }

    inline bool operator!=(const RequestID& me, const RequestID& other) {
        return !(me == other);
    }

    inline uint qHash(const RequestID& requestId) {
        return requestId.rawId();
    }

    enum class ServerEventSubscription {
        None = 0,
        AllEvents = 1,
        ServerHealthMessages = 2,
    };

    /**
        Represents a connection to a PMP server.
    */
    class ServerConnection : public QObject {
        Q_OBJECT

    private:
        enum State {
            NotConnected, Connecting, Handshake, TextMode,
            HandshakeFailure, BinaryHandshake, BinaryMode
        };

        class ResultHandler;
        class CollectionFetchResultHandler;
        class TrackInsertionResultHandler;
        class DuplicationResultHandler;
        class CompatibilityInterfaceLanguageSelectionResultHandler;
        class CompatibilityInterfaceActionTriggerResultHandler;

    public:
        enum UserRegistrationError {
            UnknownUserRegistrationError, AccountAlreadyExists, InvalidAccountName
        };

        ServerConnection(QObject* parent = nullptr,
                         ServerEventSubscription eventSubscription =
                                                      ServerEventSubscription::AllEvents);

        void reset();
        void connectToHost(QString const& host, quint16 port);

        bool isConnected() const { return _state == BinaryMode; }
        bool isLoggedIn() const { return userLoggedInId() > 0; }

        ServerHealthStatus serverHealth() const { return _serverHealthStatus; }

        QVector<int> getCompatibilityInterfaceIds();
        void sendCompatibilityInterfaceLanguageSelectionRequest(
                                                          UserInterfaceLanguage language);
        void sendCompatibilityInterfaceDefinitionsRequest(QVector<int> interfaceIds);
        RequestID sendCompatibilityInterfaceTriggerActionRequest(int interfaceId,
                                                                 int actionId);

        quint32 userLoggedInId() const;
        QString userLoggedInName() const;

        TriBool doingFullIndexation() const { return _doingFullIndexation; }

        void fetchCollection(CollectionFetcher* fetcher);

        RequestID insertQueueEntryAtIndex(FileHash const& hash, quint32 index);

        bool serverSupportsQueueEntryDuplication() const;
        bool serverSupportsDynamicModeWaveTermination() const;

    public Q_SLOTS:
        void shutdownServer();

        void sendDatabaseIdentifierRequest();
        void sendServerInstanceIdentifierRequest();
        void sendServerNameRequest();

        void requestPlayerState();
        void play();
        void pause();
        void skip();

        void seekTo(uint queueID, qint64 position);

        void insertBreakAtFront();

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
        void duplicateQueueEntry(uint queueID);
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

        void startFullIndexation();
        void requestFullIndexationRunningStatus();

    Q_SIGNALS:
        void connected();
        void cannotConnect(QAbstractSocket::SocketError error);
        void invalidServer();
        void connectionBroken(QAbstractSocket::SocketError error);
        void serverHealthChanged(ServerHealthStatus serverHealth);

        void receivedDatabaseIdentifier(QUuid uuid);
        void receivedServerInstanceIdentifier(QUuid uuid);
        void receivedServerName(quint8 nameType, QString name);

        void receivedPlayerState(PlayerState state, quint8 volume, quint32 queueLength,
                                 quint32 nowPlayingQID, quint64 nowPlayingPosition);

        void volumeChanged(int percentage);

        void dynamicModeStatusReceived(bool enabled, int noRepetitionSpanSeconds);
        void dynamicModeHighScoreWaveStatusReceived(bool active, bool statusChanged,
                                                    int progress, int progressTotal);

        void receivedPlayerHistoryEntry(PMP::PlayerHistoryTrackInfo track);
        void receivedPlayerHistory(QVector<PMP::PlayerHistoryTrackInfo> tracks);

        void receivedQueueContents(int queueLength, int startOffset,
                                   QList<quint32> queueIDs);
        void queueEntryAdded(qint32 offset, quint32 queueId, RequestID requestId);
        void queueEntryRemoved(qint32 offset, quint32 queueId);
        void queueEntryMoved(qint32 fromOffset, qint32 toOffset, quint32 queueId);
        void receivedTrackInfo(quint32 queueId, QueueEntryType type,
                               qint64 lengthMilliseconds, QString title, QString artist);
        void receivedQueueEntryHash(quint32 queueId, QueueEntryType type, FileHash hash);
        void receivedHashUserData(FileHash hash, quint32 userId,
                                  QDateTime previouslyHeard, qint16 scorePermillage);
        void receivedPossibleFilenames(quint32 queueId, QList<QString> names);

        void receivedUserAccounts(QList<QPair<uint, QString> > accounts);
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

        void compatibilityInterfaceAnnouncementReceived(QVector<int> interfaceIds);
        void compatibilityInterfaceLanguageSelectionSucceeded(
                                                          UserInterfaceLanguage language);
        void compatibilityInterfaceDefinitionReceived(int interfaceId,
                                                      CompatibilityUiState state,
                                                      UserInterfaceLanguage language,
                                                      QString title, QString caption,
                                                      QString description,
                                                      QVector<int> actionIds);
        void compatibilityInterfaceActionDefinitionReceived(int interfaceId, int actionId,
                                                         CompatibilityUiActionState state,
                                                         UserInterfaceLanguage language,
                                                         QString caption);
        void compatibilityInterfaceStateChanged(int interfaceId,
                                                CompatibilityUiState state);
        void compatibilityInterfaceTextChanged(int interfaceId,
                                               UserInterfaceLanguage language,
                                               QString caption, QString description);
        void compatibilityInterfaceActionStateChanged(int interfaceId, int actionId,
                                                      CompatibilityUiActionState state);
        void compatibilityInterfaceActionTextChanged(int interfaceId, int actionId,
                                                     UserInterfaceLanguage language,
                                                     QString caption);
        void compatibilityInterfaceActionSucceeded(int interfaceId, int actionId,
                                                   RequestID requestId);
        void compatibilityInterfaceActionFailed(int interfaceId, int actionId,
                                                RequestID requestId);

    private Q_SLOTS:
        void onConnected();
        void onReadyRead();
        void onSocketError(QAbstractSocket::SocketError error);

    private:
        uint getNewReference();

        void sendTextCommand(QString const& command);
        void sendBinaryMessage(QByteArray const& message);
        void sendProtocolExtensionsMessage();
        void sendSingleByteAction(quint8 action);

        void readTextCommands();
        void readBinaryCommands();
        void executeTextCommand(QString const& commandText);
        void handleBinaryMessage(QByteArray const& message);
        void handleStandardBinaryMessage(NetworkProtocol::ServerMessageType messageType,
                                         QByteArray const& message);
        void handleExtensionMessage(quint8 extensionId, quint8 extensionMessageType,
                                    QByteArray const& message);
        void handleResultMessage(quint16 errorType, quint32 clientReference,
                                 quint32 intData, QByteArray const& blobData);
        quint32 registerResultHandler(ResultHandler* handler);
        void discardResultHandler(quint32 clientReference);
        void registerServerProtocolExtensions(
                           const QVector<NetworkProtocol::ProtocolExtension>& extensions);

        void sendInitiateNewUserAccountMessage(QString login, quint32 clientReference);
        void sendFinishNewUserAccountMessage(QString login, QByteArray salt,
                                             QByteArray hashedPassword,
                                             quint32 clientReference);

        void handleNewUserSalt(QString login, QByteArray salt);
        void handleUserRegistrationResult(quint16 errorType, quint32 intData,
                                          QByteArray const& blobData);

        void sendInitiateLoginMessage(QString login, quint32 clientReference);
        void sendFinishLoginMessage(QString login, QByteArray userSalt,
                                    QByteArray sessionSalt, QByteArray hashedPassword,
                                    quint32 clientReference);
        void handleLoginSalt(QString login, QByteArray userSalt, QByteArray sessionSalt);
        void handleUserLoginResult(quint16 errorType, quint32 intData,
                                   QByteArray const& blobData);

        void onFullIndexationRunningStatusReceived(bool running);

        void parseSimpleResultMessage(QByteArray const& message);

        void parseServerProtocolExtensionsMessage(QByteArray const& message);
        void parseServerEventNotificationMessage(QByteArray const& message);
        void parseServerInstanceIdentifierMessage(QByteArray const& message);
        void parseServerNameMessage(QByteArray const& message);
        void parseDatabaseIdentifierMessage(QByteArray const& message);
        void parseServerHealthMessage(QByteArray const& message);

        void parseUsersListMessage(QByteArray const& message);
        void parseNewUserAccountSaltMessage(QByteArray const& message);
        void parseUserLoginSaltMessage(QByteArray const& message);

        void parsePlayerStateMessage(QByteArray const& message);
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
                                        NetworkProtocol::ServerMessageType messageType);

        void parseHashUserDataMessage(QByteArray const& message);
        void parseNewHistoryEntryMessage(QByteArray const& message);
        void parsePlayerHistoryMessage(QByteArray const& message);

        void sendCollectionFetchRequestMessage(uint clientReference);

        void parseCompatibilityInterfaceAnnouncement(QByteArray const& message);
        void parseCompatibilityInterfaceLanguageSelectionConfirmation(
                                                               QByteArray const& message);
        void parseCompatibilityInterfaceDefinition(QByteArray const& message);
        void parseCompatibilityInterfaceStateUpdate(QByteArray const& message);
        void parseCompatibilityInterfaceActionStateUpdate(QByteArray const& message);
        void parseCompatibilityInterfaceTextUpdate(QByteArray const& message);
        void parseCompatibilityInterfaceActionTextUpdate(QByteArray const& message);

        void invalidMessageReceived(QByteArray const& message, QString messageType = "",
                                    QString extraInfo = "");

        static const quint16 ClientProtocolNo;

        ServerEventSubscription _autoSubscribeToEventsAfterConnect;
        State _state;
        QTcpSocket _socket;
        QByteArray _readBuffer;
        bool _binarySendingMode;
        int _serverProtocolNo;
        QHash<quint8, QString> _serverExtensionNames;
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
        QSet<int> _compatibilityInterfaceIds;
    };
}
#endif
