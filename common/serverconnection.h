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

    public slots:
        void shutdownServer();

        void play();
        void pause();
        void skip();

        void setVolume(int percentage);

    Q_SIGNALS:
        void connected();
        void cannotConnect(QAbstractSocket::SocketError error);
        void invalidServer();
        void connectionBroken(QAbstractSocket::SocketError error);

        void playing();
        void paused();
        void stopped();

        void volumeChanged(int percentage);
        void noCurrentTrack();
        void nowPlayingTrack(QString title, QString artist, int lengthInSeconds);
        void trackPositionChanged(quint64 position);
        void queueLengthChanged(int length);

    private slots:
        void onConnected();
        void onReadyRead();
        void onSocketError(QAbstractSocket::SocketError error);
        void switchToBinaryMode();

    private:
        void readTextCommands();
        void readBinaryCommands();
        void executeTextCommand(QString const& commandText);
        void requestInitialInfo();
        void sendTextCommand(QString const& command);
        void sendBinaryMessage(QByteArray const& message);
        void sendSingleByteAction(quint8 action);
        void handleBinaryMessage(QByteArray const& message);

        State _state;
        QTcpSocket _socket;
        QByteArray _readBuffer;
        bool _binarySendingMode;
        int _serverProtocolNo;
    };
}
#endif
