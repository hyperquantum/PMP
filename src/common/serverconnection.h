/*
    Copyright (C) 2014-2018, Kevin Andre <hyperquantum@gmail.com>

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
#include "networkprotocol.h"
#include "playerhistorytrackinfo.h"
#include "simpleplayercontroller.h"
#include "tribool.h"

#include <QByteArray>
#include <QDateTime>
#include <QHash>
#include <QList>
#include <QObject>
#include <QTcpSocket>
#include <QUuid>
#include <QVector>

namespace PMP {

    class AbstractCollectionFetcher;
    class SimplePlayerControllerImpl;

    class RequestID {
    public:
        RequestID() : _rawId(0) {}
        RequestID(uint rawId) : _rawId(rawId) {}

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

    public:
        enum PlayState {
            UnknownState = 0, Stopped = 1, Playing = 2, Paused = 3
        };

        enum UserRegistrationError {
            UnknownUserRegistrationError, AccountAlreadyExists, InvalidAccountName
        };

        enum UserLoginError {
            UnknownUserLoginError, UserLoginAuthenticationFailed
        };

        ServerConnection(QObject* parent = 0, bool subscribeToAllServerEvents = true);

        void reset();
        void connectToHost(QString const& host, quint16 port);

        bool isConnected() const { return _state == BinaryMode; }
        bool isLoggedIn() const { return userLoggedInId() > 0; }

        quint32 userLoggedInId() const;
        QString userLoggedInName() const;

        TriBool doingFullIndexation() const { return _doingFullIndexation; }

        void fetchCollection(AbstractCollectionFetcher* fetcher);

        RequestID insertQueueEntryAtIndex(FileHash const& hash, quint32 index);

        SimplePlayerController& simplePlayerController();

        bool serverSupportsQueueEntryDuplication() const;

    public slots:
        void shutdownServer();

        void sendDatabaseIdentifierRequest();
        void sendServerInstanceIdentifierRequest();
        void sendServerNameRequest();

        void requestPlayerState();
        void play();
        void pause();
        void skip();

        void seekTo(uint queueID, qint64 position);

        void insertPauseAtFront();

        void setVolume(int percentage);

        void enableDynamicMode();
        void disableDynamicMode();
        void expandQueue();
        void trimQueue();
        void requestDynamicModeStatus();
        void setDynamicModeNoRepetitionSpan(int seconds);
        void startDynamicModeWave();

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

        void receivedDatabaseIdentifier(QUuid uuid);
        void receivedServerInstanceIdentifier(QUuid uuid);
        void receivedServerName(quint8 nameType, QString name);

        void playing();
        void paused();
        void stopped();
        void receivedPlayerState(int state, quint8 volume, quint32 queueLength,
                                 quint32 nowPlayingQID, quint64 nowPlayingPosition);

        void volumeChanged(int percentage);

        void dynamicModeStatusReceived(bool enabled, int noRepetitionSpan);
        void dynamicModeHighScoreWaveStatusReceived(bool active, bool statusChanged);

        void noCurrentTrack();
        void nowPlayingTrack(quint32 queueID);
        void nowPlayingTrack(QString title, QString artist, int lengthInSeconds);
        void trackPositionChanged(quint64 position);

        void receivedPlayerHistoryEntry(PMP::PlayerHistoryTrackInfo track);
        void receivedPlayerHistory(QVector<PMP::PlayerHistoryTrackInfo> tracks);

        void queueLengthChanged(int length);
        void receivedQueueContents(int queueLength, int startOffset,
                                   QList<quint32> queueIDs);
        void queueEntryAdded(quint32 offset, quint32 queueID, RequestID requestID);
        void queueEntryRemoved(quint32 offset, quint32 queueID);
        void queueEntryMoved(quint32 fromOffset, quint32 toOffset, quint32 queueID);
        void receivedTrackInfo(quint32 queueID, QueueEntryType type, int lengthInSeconds,
                               QString title, QString artist);
        void receivedQueueEntryHash(quint32 queueID, QueueEntryType type, FileHash hash);
        void receivedHashUserData(FileHash hash, quint32 userId,
                                  QDateTime previouslyHeard, qint16 scorePermillage);
        void receivedPossibleFilenames(quint32 queueID, QList<QString> names);

        void receivedUserAccounts(QList<QPair<uint, QString> > accounts);
        void userAccountCreatedSuccessfully(QString login, quint32 id);
        void userAccountCreationError(QString login, UserRegistrationError errorType);

        void userLoggedInSuccessfully(QString login, quint32 id);
        void userLoginError(QString login, UserLoginError errorType);

        void receivedUserPlayingFor(quint32 userId, QString userLogin);

        void fullIndexationStatusReceived(bool running);
        void fullIndexationStarted();
        void fullIndexationFinished();

        void collectionTracksChanged(QList<PMP::CollectionTrackInfo> changes);

    private slots:
        void onConnected();
        void onReadyRead();
        void onSocketError(QAbstractSocket::SocketError error);

    private:
        uint getNewReference();

        void sendTextCommand(QString const& command);
        void sendBinaryMessage(QByteArray const& message);
        void sendSingleByteAction(quint8 action);

        void readTextCommands();
        void readBinaryCommands();
        void executeTextCommand(QString const& commandText);
        void handleBinaryMessage(QByteArray const& message);
        void handleResultMessage(quint16 errorType, quint32 clientReference,
                                 quint32 intData, QByteArray const& blobData);

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

        void parseTrackInfoBatchMessage(QByteArray const& message,
                                        NetworkProtocol::ServerMessageType messageType);

        void parseBulkQueueEntryHashMessage(QByteArray const& message);
        void parseHashUserDataMessage(QByteArray const& message);
        void parseNewHistoryEntryMessage(QByteArray const& message);
        void parsePlayerHistoryMessage(QByteArray const& message);

        void parseDynamicModeWaveStatusMessage(QByteArray const& message);

        void parseQueueEntryAddedMessage(QByteArray const& message);
        void parseQueueEntryAdditionConfirmationMessage(QByteArray const& message);

        void sendCollectionFetchRequestMessage(uint clientReference);

        void invalidMessageReceived(QByteArray const& message, QString messageType = "",
                                    QString extraInfo = "");

        static const qint16 ClientProtocolNo;

        bool _autoSubscribeToEventsAfterConnect;
        State _state;
        QTcpSocket _socket;
        QByteArray _readBuffer;
        bool _binarySendingMode;
        int _serverProtocolNo;
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
        QHash<uint, AbstractCollectionFetcher*> _collectionFetchers;
        SimplePlayerControllerImpl* _simplePlayerController;
    };

    class AbstractCollectionFetcher : public QObject {
        Q_OBJECT
    protected:
        AbstractCollectionFetcher(QObject* parent = 0);

    public slots:
        virtual void receivedData(QList<CollectionTrackInfo> data) = 0;
        virtual void completed() = 0;
        virtual void errorOccurred() = 0;
    };

    class SimplePlayerControllerImpl : public QObject, public SimplePlayerController {
        Q_OBJECT
    public:
        SimplePlayerControllerImpl(ServerConnection* connection);

        ~SimplePlayerControllerImpl() {}

        void play();
        void pause();
        void skip();

        bool canPlay();
        bool canPause();
        bool canSkip();

    private slots:
        void receivedPlayerState(int state, quint8 volume, quint32 queueLength,
                                 quint32 nowPlayingQID, quint64 nowPlayingPosition);

    private:
        ServerConnection* _connection;
        ServerConnection::PlayState _state;
        uint _queueLength;
        quint32 _trackNowPlaying;
        quint32 _trackJustSkipped;
    };
}
#endif
