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

#ifndef PMP_CONNECTEDCLIENT_H
#define PMP_CONNECTEDCLIENT_H

#include "common/networkprotocol.h"

#include "player.h" /* for the State enum :( */

#include <QByteArray>
#include <QList>
#include <QTcpSocket>

namespace PMP {

    class Generator;
    class QueueEntry;
    class Server;
    class Users;

    class ConnectedClient : public QObject {
        Q_OBJECT
    public:
        ConnectedClient(QTcpSocket* socket, Server* server, Player* player,
                        Generator* generator, Users* users);

    Q_SIGNALS:

    private slots:

        void terminateConnection();
        void dataArrived();
        void socketError(QAbstractSocket::SocketError error);

        void volumeChanged(int volume);
        void dynamicModeStatusChanged(bool enabled);
        void dynamicModeNoRepetitionSpanChanged(int seconds);
        void playerStateChanged(Player::State state);
        void currentTrackChanged(QueueEntry const* entry);
        void trackPositionChanged(qint64 position);
        void sendStateInfo();
        void sendVolumeMessage();
        void sendDynamicModeStatusMessage();
        void sendUserPlayingForModeMessage();
        void sendTextualQueueInfo();
        void queueEntryRemoved(quint32 offset, quint32 queueID);
        void queueEntryAdded(quint32 offset, quint32 queueID);
        void queueEntryMoved(quint32 fromOffset, quint32 toOffset, quint32 queueID);

    private:
        void readTextCommands();
        void readBinaryCommands();
        void executeTextCommand(QString const& commandText);
        void sendTextCommand(QString const& command);
        void sendBinaryMessage(QByteArray const& message);
        void sendServerInstanceIdentifier();
        void sendUsersList();
        void sendQueueContentMessage(quint32 startOffset, quint8 length);
        void sendQueueEntryRemovedMessage(quint32 offset, quint32 queueID);
        void sendQueueEntryAddedMessage(quint32 offset, quint32 queueID);
        void sendQueueEntryMovedMessage(quint32 fromOffset, quint32 toOffset,
                                        quint32 queueID);
        void sendTrackInfoMessage(quint32 queueID);
        void sendTrackInfoMessage(QList<quint32> const& queueIDs);
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
        void sendUserLoginSaltMessage(QString login, QByteArray const& userSalt,
                                      QByteArray const& sessionSalt);
        void handleBinaryMessage(QByteArray const& message);

        bool _terminated;
        QTcpSocket* _socket;
        Server* _server;
        Player* _player;
        Generator* _generator;
        Users* _users;
        QByteArray _textReadBuffer;
        bool _binaryMode;
        int _clientProtocolNo;
        quint32 _lastSentNowPlayingID;
        QString _userAccountRegistering;
        QByteArray _saltForUserAccountRegistering;
        QString _userAccountLoggingIn;
        QByteArray _sessionSaltForUserLoggingIn;
        quint32 _userLoggedIn;
        QString _userLoggedInName;
    };
}
#endif
