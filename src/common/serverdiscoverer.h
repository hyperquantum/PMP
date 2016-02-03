/*
    Copyright (C) 2016, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_SERVERDISCOVERER_H
#define PMP_SERVERDISCOVERER_H

#include <QHash>
#include <QHostAddress>
#include <QObject>
#include <QPair>
#include <QPointer>
#include <QUuid>

QT_FORWARD_DECLARE_CLASS(QUdpSocket)

namespace PMP {

    class ServerConnection;
    class ServerProbe;

    class ServerDiscoverer : public QObject {
        Q_OBJECT
    public:
        ServerDiscoverer(QObject* parent = nullptr);
        ~ServerDiscoverer();

    public slots:
        void sendProbe();

    Q_SIGNALS:
        void foundServer(QHostAddress address, quint16 port, QUuid id, QString name);
        void foundExtraServerAddress(QHostAddress address, QUuid id);

    private slots:
        void sendProbeToLocalhost();
        void sendBroadcastProbe();
        void readPendingDatagrams();
        void onFoundServer(QHostAddress address, quint16 port,
                           QUuid serverId, QString name);

    private:
        void sendProbeTo(QHostAddress const& destination);
        void receivedProbeReply(QHostAddress const& server, quint16 port);

        struct ServerData {
            quint16 port;
            QList<QHostAddress> addresses;
            QString name;
        };

        QList<QHostAddress> _localHostNetworkAddresses;
        QUdpSocket* _socket;
        QHash<QPair<QHostAddress, quint16>, QPointer<ServerProbe> > _addresses;
        QHash<QUuid, ServerData*> _servers;
    };

    class ServerProbe : public QObject {
        Q_OBJECT
    public:
        ServerProbe(QObject* parent, QHostAddress const& address, quint16 port);

    Q_SIGNALS:
        void foundServer(QHostAddress address, quint16 port,
                         QUuid serverId, QString name);

    private slots:
        void onConnected();
        void onReceivedServerUuid(QUuid uuid);
        void onReceivedServerName(quint8 nameType, QString name);
        void onTimeout();

    private:
        void emitSignalIfDataComplete();

        QHostAddress _address;
        quint16 _port;
        ServerConnection* _connection;
        QUuid _serverId;
        QString _serverName;
        uint _serverNameType;
    };
}
#endif
