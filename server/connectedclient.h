/*
    Copyright (C) 2014, Kevin Andre <hyperquantum@gmail.com>

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

#include "player.h" /* for the State enum :( */

#include <QByteArray>
#include <QList>
#include <QTcpSocket>

namespace PMP {

    class Generator;
    class QueueEntry;
    class Server;

    class ConnectedClient : public QObject {
        Q_OBJECT
    public:
        ConnectedClient(QTcpSocket* socket, Server* server, Player* player, Generator* generator);

    Q_SIGNALS:

    private slots:

        void terminateConnection();
        void dataArrived();
        void socketError(QAbstractSocket::SocketError error);

        void volumeChanged(int volume);
        void dynamicModeStatusChanged(bool enabled);
        void playerStateChanged(Player::State state);
        void currentTrackChanged(QueueEntry const* entry);
        void trackPositionChanged(qint64 position);
        void sendStateInfo();
        void sendVolumeMessage();
        void sendDynamicModeStatusMessage();
        void sendTextualQueueInfo();
        void queueEntryRemoved(quint32 offset, quint32 queueID);
        void queueEntryAdded(quint32 offset, quint32 queueID);

    private:
        void readTextCommands();
        void readBinaryCommands();
        void executeTextCommand(QString const& commandText);
        void sendTextCommand(QString const& command);
        void sendBinaryMessage(QByteArray const& message);
        void sendQueueContentMessage(quint32 startOffset, quint8 length);
        void sendQueueEntryRemovedMessage(quint32 offset, quint32 queueID);
        void sendQueueEntryAddedMessage(quint32 offset, quint32 queueID);
        void sendTrackInfoMessage(quint32 queueID);
        void sendTrackInfoMessage(QList<quint32> const& queueIDs);
        void handleBinaryMessage(QByteArray const& message);

        QTcpSocket* _socket;
        Server* _server;
        Player* _player;
        Generator* _generator;
        QByteArray _textReadBuffer;
        bool _binaryMode;
        int _clientProtocolNo;
        quint32 _lastSentNowPlayingID;
    };
}
#endif
