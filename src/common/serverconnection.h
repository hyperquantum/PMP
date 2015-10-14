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

#ifndef PMP_SERVERCONNECTION_H
#define PMP_SERVERCONNECTION_H

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QTcpSocket>
#include <QUuid>

namespace PMP {

    /**
        Represents a connection to a PMP server.
    */
    class ServerConnection : public QObject {
        Q_OBJECT

        enum State {
            NotConnected, Connecting, Handshake, TextMode,
            HandshakeFailure, BinaryHandshake, BinaryMode
        };
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

        ServerConnection(QObject* parent = 0);

        void reset();
        void connectToHost(QString const& host, quint16 port);

        bool isConnected() const { return _state == BinaryMode; }

        quint32 userLoggedInId() const;
        QString userLoggedInName() const;

    public slots:
        void shutdownServer();

        void sendServerInstanceIdentifierRequest();

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

        void sendQueueFetchRequest(uint startOffset, quint8 length = 0);
        void deleteQueueEntry(uint queueID);
        void moveQueueEntry(uint queueID, qint16 offsetDiff);

        void sendTrackInfoRequest(uint queueID);
        void sendTrackInfoRequest(QList<uint> const& queueIDs);

        void sendPossibleFilenamesRequest(uint queueID);

        void sendUserAccountsFetchRequest();
        void createNewUserAccount(QString login, QString password);
        void login(QString login, QString password);

        void switchToPublicMode();
        void switchToPersonalMode();
        void requestUserPlayingForMode();

        void startFullIndexation();

    Q_SIGNALS:
        void connected();
        void cannotConnect(QAbstractSocket::SocketError error);
        void invalidServer();
        void connectionBroken(QAbstractSocket::SocketError error);

        void receivedServerInstanceIdentifier(QUuid uuid);

        void playing();
        void paused();
        void stopped();
        void receivedPlayerState(int state, quint8 volume, quint32 queueLength,
                                 quint32 nowPlayingQID, quint64 nowPlayingPosition);

        void volumeChanged(int percentage);

        void dynamicModeStatusReceived(bool enabled, int noRepetitionSpan);

        void noCurrentTrack();
        void nowPlayingTrack(quint32 queueID);
        void nowPlayingTrack(QString title, QString artist, int lengthInSeconds);
        void trackPositionChanged(quint64 position);
        void queueLengthChanged(int length);
        void receivedQueueContents(int queueLength, int startOffset,
                                   QList<quint32> queueIDs);
        void queueEntryAdded(quint32 offset, quint32 queueID);
        void queueEntryRemoved(quint32 offset, quint32 queueID);
        void queueEntryMoved(quint32 fromOffset, quint32 toOffset, quint32 queueID);
        void receivedTrackInfo(quint32 queueID, int lengthInSeconds, QString title,
                               QString artist);
        void receivedPossibleFilenames(quint32 queueID, QList<QString> names);

        void receivedUserAccounts(QList<QPair<uint, QString> > accounts);
        void userAccountCreatedSuccessfully(QString login, quint32 id);
        void userAccountCreationError(QString login, UserRegistrationError errorType);

        void userLoggedInSuccessfully(QString login, quint32 id);
        void userLoginError(QString login, UserLoginError errorType);

        void receivedUserPlayingFor(quint32 userId, QString userLogin);

    private slots:
        void onConnected();
        void onReadyRead();
        void onSocketError(QAbstractSocket::SocketError error);

    private:
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
    };
}
#endif
