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

#ifndef PMP_SERVER_H
#define PMP_SERVER_H

#include <QHostAddress>
#include <QSet>
#include <QUuid>

QT_FORWARD_DECLARE_CLASS(QTcpServer)
QT_FORWARD_DECLARE_CLASS(QTimer)
QT_FORWARD_DECLARE_CLASS(QUdpSocket)

namespace PMP {

    class CollectionMonitor;
    class Generator;
    class History;
    class Player;
    class Scrobbling;
    class ServerHealthMonitor;
    class Users;

    class Server : public QObject {
        Q_OBJECT
    public:
        Server(QObject* parent, const QUuid& serverInstanceIdentifier);

        bool listen(Player* player, Generator* generator, History* history,
                    Users* users,
                    CollectionMonitor* collectionMonitor,
                    ServerHealthMonitor* serverHealthMonitor,
                    Scrobbling* scrobbling,
                    const QHostAddress& address = QHostAddress::Any, quint16 port = 0);

        QString errorString() const;

        quint16 port() const;

        QUuid uuid() const { return _uuid; }
        QString serverPassword() { return _serverPassword; }

    public Q_SLOTS:
        void shutdown();

    Q_SIGNALS:
        void shuttingDown();

    private Q_SLOTS:
        void newConnectionReceived();
        void sendBroadcast();
        void readPendingDatagrams();

    private:
        uint generateConnectionReference();
        void retireConnectionReference(uint connectionReference);
        static QString generateServerPassword();

        uint _lastNewConnectionReference;
        QSet<uint> _connectionReferencesInUse;
        QUuid _uuid;
        QString _serverPassword;
        Player* _player;
        Generator* _generator;
        History* _history;
        Users* _users;
        CollectionMonitor* _collectionMonitor;
        ServerHealthMonitor* _serverHealthMonitor;
        Scrobbling* _scrobbling;
        QTcpServer* _server;
        QUdpSocket* _udpSocket;
        QTimer* _broadcastTimer;
    };
}
#endif
