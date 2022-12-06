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

#ifndef PMP_TCPSERVER_H
#define PMP_TCPSERVER_H

#include <QHostAddress>
#include <QUuid>

QT_FORWARD_DECLARE_CLASS(QTcpServer)
QT_FORWARD_DECLARE_CLASS(QTimer)
QT_FORWARD_DECLARE_CLASS(QUdpSocket)

namespace PMP
{
    class CollectionMonitor;
    class DelayedStart;
    class Generator;
    class HashIdRegistrar;
    class History;
    class Player;
    class ServerHealthMonitor;
    class ServerInterface;
    class ServerSettings;
    class Users;

    class TcpServer : public QObject
    {
        Q_OBJECT
    public:
        TcpServer(QObject* parent, ServerSettings* serverSettings,
               const QUuid& serverInstanceIdentifier);

        bool listen(Player* player, Generator* generator, History* history,
                    HashIdRegistrar* hashIdRegistrar,
                    Users* users,
                    CollectionMonitor* collectionMonitor,
                    ServerHealthMonitor* serverHealthMonitor,
                    DelayedStart* delayedStart,
                    const QHostAddress& address = QHostAddress::Any, quint16 port = 0);

        QString errorString() const;

        quint16 port() const;

        QUuid uuid() const { return _uuid; }
        QString caption() const { return _caption; }
        QString serverPassword() const { return _serverPassword; }

    public Q_SLOTS:
        void shutdown();

    Q_SIGNALS:
        void captionChanged();
        void serverClockTimeSendingPulse();
        void shuttingDown();

    private Q_SLOTS:
        void newConnectionReceived();
        void sendBroadcast();
        void readPendingDatagrams();

    private:
        static QString generateServerPassword();

        ServerInterface* createServerInterface();
        void determineCaption();

        QUuid _uuid;
        QString _caption;
        QString _serverPassword;
        ServerSettings* _settings;
        Player* _player;
        Generator* _generator;
        History* _history;
        HashIdRegistrar* _hashIdRegistrar { nullptr };
        Users* _users;
        CollectionMonitor* _collectionMonitor;
        ServerHealthMonitor* _serverHealthMonitor;
        DelayedStart* _delayedStart;
        QTcpServer* _server;
        QUdpSocket* _udpSocket;
        QTimer* _broadcastTimer;
        int _connectionCount;
    };
}
#endif
