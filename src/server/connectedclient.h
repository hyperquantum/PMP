/*
    Copyright (C) 2014-2016, Kevin Andre <hyperquantum@gmail.com>

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

#include "playerstate.h"

#include <QByteArray>
#include <QList>
#include <QTcpSocket>

namespace PMP {

    class CollectionMonitor;
    class CollectionSender;
    class Generator;
    class Player;
    class QueueEntry;
    class Resolver;
    class Server;
    class Users;

    class ConnectedClient : public QObject {
        Q_OBJECT
    public:
        ConnectedClient(QTcpSocket* socket, Server* server, Player* player,
                        Generator* generator, Users* users,
                        CollectionMonitor* collectionMonitor);

        ~ConnectedClient();

    Q_SIGNALS:

    private slots:

        void terminateConnection();
        void dataArrived();
        void socketError(QAbstractSocket::SocketError error);

        void volumeChanged(int volume);
        void dynamicModeStatusChanged(bool enabled);
        void dynamicModeNoRepetitionSpanChanged(int seconds);
        void playerStateChanged(PlayerState state);
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
        void onUserPlayingForChanged(quint32 user);
        void onFullIndexationRunStatusChanged(bool running);
        void onCollectionTrackInfoBatchToSend(uint clientReference,
                                              QList<CollectionTrackInfo> tracks);
        void onCollectionTrackInfoCompleted(uint clientReference);
        void onHashAvailabilityChanged(QList<QPair<PMP::FileHash, bool> > changes);
        void onHashInfoChanged(QList<PMP::CollectionTrackInfo> changes);

    private:
        bool isLoggedIn() const;

        void readTextCommands();
        void readBinaryCommands();
        void executeTextCommand(QString const& commandText);
        void sendTextCommand(QString const& command);
        void sendBinaryMessage(QByteArray const& message);
        void sendEventNotificationMessage(quint8 event);
        void sendServerInstanceIdentifier();
        void sendUsersList();
        void sendQueueContentMessage(quint32 startOffset, quint8 length);
        void sendQueueEntryRemovedMessage(quint32 offset, quint32 queueID);
        void sendQueueEntryAddedMessage(quint32 offset, quint32 queueID);
        void sendQueueEntryMovedMessage(quint32 fromOffset, quint32 toOffset,
                                        quint32 queueID);
        void sendQueueEntryInfoMessage(quint32 queueID);
        void sendQueueEntryInfoMessage(QList<quint32> const& queueIDs);
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
        void sendTrackInfoBatchMessage(uint clientReference, bool isNotification,
                                       QList<CollectionTrackInfo> tracks);
        void sendServerNameMessage(quint8 type, QString name);
        void handleBinaryMessage(QByteArray const& message);
        void handleSingleByteAction(quint8 action);
        void handleCollectionFetchRequest(uint clientReference);
        void parseAddHashToQueueRequest(QByteArray const& message,
                                        NetworkProtocol::ClientMessageType messageType);

        bool _terminated;
        QTcpSocket* _socket;
        Server* _server;
        Player* _player;
        Generator* _generator;
        Users* _users;
        CollectionMonitor* _collectionMonitor;
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

    class CollectionSender : public QObject {
        Q_OBJECT
    public:
        CollectionSender(ConnectedClient* connection, uint clientReference,
                         Resolver* resolver);

    Q_SIGNALS:
        void sendCollectionList(uint clientReference, QList<CollectionTrackInfo> tracks);
        void allSent(uint clientReference);

    private slots:
        void sendNextBatch();

    private:
        uint _clientRef;
        Resolver* _resolver;
        QList<FileHash> _hashes;
        int _currentIndex;
    };

}
#endif
