/*
    Copyright (C) 2016-2022, Kevin Andre <hyperquantum@gmail.com>

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
#include <QSet>
#include <QSharedPointer>
#include <QUuid>

QT_FORWARD_DECLARE_CLASS(QUdpSocket)

namespace PMP
{
    class ServerConnection;
    class ServerProbe;

    class ServerDiscoverer : public QObject
    {
        Q_OBJECT
    public:
        explicit ServerDiscoverer(QObject* parent = nullptr);
        ~ServerDiscoverer();

        bool canDoScan() const;

    public Q_SLOTS:
        void scanForServers();

    Q_SIGNALS:
        void canDoScanChanged();
        void foundServer(QHostAddress address, quint16 port, QUuid id, QString name);
        void foundExtraServerAddress(QHostAddress address, QUuid id);

    private Q_SLOTS:
        void sendProbeToLocalhost();
        void sendBroadcastProbe();
        void readPendingDatagrams();
        void onFoundServer(QHostAddress address, quint16 port,
                           QUuid serverId, QString name);

    private:
        bool isLocalhostAddress(QHostAddress const& address);
        void sendProbeTo(QHostAddress const& destination);
        void receivedServerAnnouncement(QHostAddress const& server, quint16 port);

        struct ServerData
        {
            quint16 port;
            QList<QHostAddress> addresses;
            QString name;
        };

        QList<QHostAddress> _localHostNetworkAddresses;
        QUdpSocket* _socket;
        QSet<QPair<QHostAddress, quint16>> _addressesBeingProbed;
        QHash<QUuid, QSharedPointer<ServerData>> _servers;
        bool _scanInProgress;
    };

    class ServerProbe : public QObject
    {
        Q_OBJECT
    public:
        ServerProbe(QObject* parent, QHostAddress const& address, quint16 port);

    Q_SIGNALS:
        void foundServer(QHostAddress address, quint16 port,
                         QUuid serverId, QString name);

    private Q_SLOTS:
        void onConnected();
        void onReceivedServerUuid(QUuid uuid);
        void onReceivedServerName(quint8 nameType, QString name);
        void onTimeout();

    private:
        void emitSignalIfDataComplete();
        void cleanUpConnection();

        QHostAddress _address;
        quint16 _port;
        ServerConnection* _connection;
        QUuid _serverId;
        QString _serverName;
        uint _serverNameType;
    };
}
#endif
