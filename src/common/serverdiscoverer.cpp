/*
    Copyright (C) 2016-2020, Kevin Andre <hyperquantum@gmail.com>

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

#include "serverdiscoverer.h"

#include "networkutil.h"
#include "serverconnection.h"

#include <QByteArray>
#include <QNetworkInterface>
#include <QtDebug>
#include <QTimer>
#include <QUdpSocket>

namespace PMP {

    ServerDiscoverer::ServerDiscoverer(QObject* parent)
     : QObject(parent), _socket(new QUdpSocket(this))
    {
        _localHostNetworkAddresses = QNetworkInterface::allAddresses();
        qDebug() << "all network addresses:" << _localHostNetworkAddresses;

        bool bound =
            _socket->bind(
                23433, QAbstractSocket::ShareAddress | QAbstractSocket::ReuseAddressHint
              );
        if (!bound) { qDebug() << "ServerDiscoverer: BIND FAILED"; }

        connect(
            _socket, &QUdpSocket::readyRead, this, &ServerDiscoverer::readPendingDatagrams
        );
    }

    ServerDiscoverer::~ServerDiscoverer() {
        qDeleteAll(_addresses.values());
        qDeleteAll(_servers.values());
    }

    void ServerDiscoverer::sendProbe() {
        /* first send to localhost and then broadcast */
        sendProbeToLocalhost();
        QTimer::singleShot(100, this, &ServerDiscoverer::sendBroadcastProbe);
    }

    void ServerDiscoverer::sendProbeToLocalhost() {
        sendProbeTo(QHostAddress::LocalHost);
        sendProbeTo(QHostAddress::LocalHostIPv6);
    }

    void ServerDiscoverer::sendBroadcastProbe() {
        sendProbeTo(QHostAddress::Broadcast);
    }

    void ServerDiscoverer::sendProbeTo(QHostAddress const& destination) {
        QByteArray datagram = "PMPPROBEv01";
        _socket->writeDatagram(datagram, destination, 23432);
        _socket->flush();
    }

    void ServerDiscoverer::readPendingDatagrams() {
        while (_socket->hasPendingDatagrams()) {
            QByteArray datagram;
            datagram.resize(_socket->pendingDatagramSize());
            QHostAddress sender;
            quint16 senderPort;

            _socket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

            if (datagram.size() < 23 || !datagram.startsWith("PMPSERVERANNOUNCEv01 "))
                continue;

            quint16 serverListeningPort = NetworkUtil::get2Bytes(datagram, 21);

            qDebug() << "ServerDiscoverer: received announcement from" << sender
                     << "origin port" << senderPort
                     << "; server active on port" << serverListeningPort;

            receivedProbeReply(sender, serverListeningPort);
        }
    }

    void ServerDiscoverer::receivedProbeReply(const QHostAddress& server, quint16 port) {
        auto serverAndPort = qMakePair(server, port);

        if (_addresses.contains(serverAndPort))
            return; /* already (being) handled */

        /* The sender of the datagram we receive is never 127.0.0.1 or ::1, but instead
         * the IPv4 or IPv6 address of the host on the network. We want to detect that
         * situation, because we prefer connecting to the server through the host's
         * loopback interface. */

        bool isFromLocalhost =
            server == QHostAddress::LocalHost || server == QHostAddress::LocalHostIPv6;

        if (!isFromLocalhost) {
            Q_FOREACH(auto localAddress, _localHostNetworkAddresses) {
                if (localAddress.isEqual(server, QHostAddress::TolerantConversion))
                {
                   isFromLocalhost = true;
                   break;
                }
            }
        }

        qDebug() << "Originated from localhost?" << (isFromLocalhost ? "Yes" : "No");

        if (isFromLocalhost) {
            auto addressToUse =
                server.protocol() == QAbstractSocket::IPv4Protocol
                    ? QHostAddress(QHostAddress::LocalHost)
                    : QHostAddress(QHostAddress::LocalHostIPv6);

            serverAndPort = qMakePair(addressToUse, port);
        }

        if (_addresses.contains(serverAndPort))
            return; /* already (being) handled */

        auto probe = new ServerProbe(this, serverAndPort.first, serverAndPort.second);
        connect(probe, &ServerProbe::foundServer, this, &ServerDiscoverer::onFoundServer);
        _addresses[serverAndPort] = probe;
    }

    void ServerDiscoverer::onFoundServer(QHostAddress address, quint16 port,
                                         QUuid serverId, QString name)
    {
        bool isNew = false;
        auto serverData = _servers.value(serverId, nullptr);
        if (!serverData) {
            isNew = true;
            serverData = new ServerData();
            serverData->port = port;
            serverData->name = name;
            _servers[serverId] = serverData;
        }

        bool newAddress = false;
        if (isNew || !serverData->addresses.contains(address)) {
            newAddress = !isNew;
            serverData->addresses.append(address);
        }

        if (isNew) {
            emit foundServer(address, port, serverId, name);
        }
        else if (newAddress) {
            emit foundExtraServerAddress(address, serverId);
        }
    }

    // ============================================================================== //

    ServerProbe::ServerProbe(QObject* parent, QHostAddress const& address, quint16 port)
     : QObject(parent),
       _address(address), _port(port),
       _connection(new ServerConnection(this,
                                          ServerEventSubscription::ServerHealthMessages)),
       _serverNameType(0)
    {
        qDebug() << "ServerProbe created for" << address << "and port" << port;

        connect(
            _connection, &ServerConnection::connected,
            this, &ServerProbe::onConnected
        );
        connect(
            _connection, &ServerConnection::receivedServerInstanceIdentifier,
            this, &ServerProbe::onReceivedServerUuid
        );
        connect(
            _connection, &ServerConnection::receivedServerName,
            this, &ServerProbe::onReceivedServerName
        );

        _connection->connectToHost(address.toString(), port);

        /* set a timer to avoid waiting indefinitely */
        QTimer::singleShot(4000, this, &ServerProbe::onTimeout);
    }

    void ServerProbe::onConnected() {
        _connection->sendServerInstanceIdentifierRequest();
        _connection->sendServerNameRequest();
    }

    void ServerProbe::onReceivedServerUuid(QUuid uuid) {
        _serverId = uuid;
        emitSignalIfDataComplete();
    }

    void ServerProbe::onReceivedServerName(quint8 nameType, QString name) {
        if (_serverNameType > nameType || _serverName == name || name == "")
            return;

        _serverNameType = nameType;
        _serverName = name;

        emitSignalIfDataComplete();
    }

    void ServerProbe::onTimeout() {
        if (_connection == nullptr)
            return; /* already cleaned up */

        qDebug() << "ServerProbe: TIMEOUT for" << _address << "port" << _port;

        cleanupConnection();

        if (!_serverId.isNull()) { /* server found but did not receive a name? */
            /* send with empty name */
            emit foundServer(_address, _port, _serverId, _serverName);
        }
    }

    void ServerProbe::emitSignalIfDataComplete() {
        if (_serverId.isNull() || _serverName == "")
            return; /* not yet complete */

        cleanupConnection();
        emit foundServer(_address, _port, _serverId, _serverName);
    }

    void ServerProbe::cleanupConnection() {
        _connection->reset();
        _connection->deleteLater();
        _connection = nullptr;
    }
}
