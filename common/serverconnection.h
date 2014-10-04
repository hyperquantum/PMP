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

#ifndef PMP_SERVERCONNECTION_H
#define PMP_SERVERCONNECTION_H

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QTcpSocket>

namespace PMP {

    class ServerConnection : public QObject {
        Q_OBJECT

        enum State {
            NotConnected, Connecting, Handshake, TextMode,
            HandshakeFailure, BinaryHandshake, BinaryMode
        };
    public:
        ServerConnection(QObject* parent = 0);

        void reset();
        void connectToHost(QString const& host, quint16 port);

        bool isConnected() const { return _state == BinaryMode; }

    public slots:
        void shutdownServer();

        void play();
        void pause();
        void skip();

        void setVolume(int percentage);

        void enableDynamicMode();
        void disableDynamicMode();
        void requestDynamicModeStatus();

        void sendQueueFetchRequest(uint startOffset, quint8 length = 0);
        void deleteQueueEntry(uint queueID);

        void sendTrackInfoRequest(uint queueID);
        void sendTrackInfoRequest(QList<uint> const& queueIDs);

    Q_SIGNALS:
        void connected();
        void cannotConnect(QAbstractSocket::SocketError error);
        void invalidServer();
        void connectionBroken(QAbstractSocket::SocketError error);

        void playing();
        void paused();
        void stopped();

        void volumeChanged(int percentage);

        void dynamicModeEnabled();
        void dynamicModeDisabled();

        void noCurrentTrack();
        void nowPlayingTrack(quint32 queueID);
        void nowPlayingTrack(QString title, QString artist, int lengthInSeconds);
        void trackPositionChanged(quint64 position);
        void queueLengthChanged(int length);
        void receivedQueueContents(int queueLength, int startOffset, QList<quint32> queueIDs);
        void queueEntryAdded(quint32 offset, quint32 queueID);
        void queueEntryRemoved(quint32 offset, quint32 queueID);
        void receivedTrackInfo(quint32 queueID, int lengthInSeconds, QString title, QString artist);

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

        State _state;
        QTcpSocket _socket;
        QByteArray _readBuffer;
        bool _binarySendingMode;
        int _serverProtocolNo;
    };
}
#endif
