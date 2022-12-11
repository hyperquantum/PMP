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

#include "serverdiscoverer.h"

#include "common/networkutil.h"

#include "serverconnection.h"

#include <QByteArray>
#include <QNetworkInterface>
#include <QtDebug>
#include <QTimer>
#include <QUdpSocket>

namespace PMP
{
    ServerDiscoverer::ServerDiscoverer(QObject* parent)
     : QObject(parent),
       _socket(new QUdpSocket(this)),
       _scanInProgress(false)
    {
        _localHostNetworkAddresses = QNetworkInterface::allAddresses();
        qDebug() << "all network addresses of localhost:" << _localHostNetworkAddresses;

        connect(
            _socket, &QUdpSocket::readyRead, this, &ServerDiscoverer::readPendingDatagrams
        );

        bool bound =
            _socket->bind(
                23433, QAbstractSocket::ShareAddress | QAbstractSocket::ReuseAddressHint
              );
        if (!bound) { qDebug() << "ServerDiscoverer: BIND FAILED"; }
    }

    ServerDiscoverer::~ServerDiscoverer()
    {
        //
    }

    bool ServerDiscoverer::canDoScan() const
    {
        return !_scanInProgress;
    }

    void ServerDiscoverer::scanForServers()
    {
        if (!canDoScan())
            return;

        _localHostNetworkAddresses = QNetworkInterface::allAddresses();
        qDebug() << "all network addresses of localhost:" << _localHostNetworkAddresses;

        /* first send to localhost and then broadcast */
        sendProbeToLocalhost();
        QTimer::singleShot(100, this, &ServerDiscoverer::sendBroadcastProbe);

        _scanInProgress = true;
        QTimer::singleShot(
            10 * 1000, this,
            [this]()
            {
                _scanInProgress = false;
                Q_EMIT canDoScanChanged();
            }
        );
        Q_EMIT canDoScanChanged();
    }

    void ServerDiscoverer::sendProbeToLocalhost()
    {
        sendProbeTo(QHostAddress::LocalHost);
        sendProbeTo(QHostAddress::LocalHostIPv6);
    }

    void ServerDiscoverer::sendBroadcastProbe()
    {
        sendProbeTo(QHostAddress::Broadcast);
    }

    void ServerDiscoverer::sendProbeTo(QHostAddress const& destination)
    {
        QByteArray datagram = "PMPPROBEv01";
        _socket->writeDatagram(datagram, destination, 23432);
        _socket->flush();
    }

    void ServerDiscoverer::readPendingDatagrams()
    {
        while (_socket->hasPendingDatagrams())
        {
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

            receivedServerAnnouncement(sender, serverListeningPort);
        }
    }

    void ServerDiscoverer::receivedServerAnnouncement(const QHostAddress& server,
                                                      quint16 port)
    {
        auto serverAndPort = qMakePair(server, port);

        if (_addressesBeingProbed.contains(serverAndPort))
            return; /* already being handled */

        /* The sender of the datagram we receive is never 127.0.0.1 or ::1, but instead
         * the IPv4 or IPv6 address of the host on the network. We want to detect that
         * situation, because we prefer connecting to the server through the host's
         * loopback interface. */

        bool isFromLocalhost = isLocalhostAddress(server);

        qDebug() << "Originated from localhost?" << (isFromLocalhost ? "Yes" : "No");

        if (isFromLocalhost)
        {
            auto addressToUse =
                server.protocol() == QAbstractSocket::IPv4Protocol
                    ? QHostAddress(QHostAddress::LocalHost)
                    : QHostAddress(QHostAddress::LocalHostIPv6);

            serverAndPort = qMakePair(addressToUse, port);
        }

        if (_addressesBeingProbed.contains(serverAndPort))
            return; /* already (being) handled */

        _addressesBeingProbed << serverAndPort;
        auto probe = new ServerProbe(this, serverAndPort.first, serverAndPort.second);
        connect(probe, &ServerProbe::foundServer, this, &ServerDiscoverer::onFoundServer);
        connect(
            probe, &ServerProbe::destroyed,
            this,
            [this, serverAndPort]() { _addressesBeingProbed.remove(serverAndPort); }
        );
    }

    void ServerDiscoverer::onFoundServer(QHostAddress address, quint16 port,
                                         QUuid serverId, QString name)
    {
        bool isNewServer = false;
        auto serverData = _servers.value(serverId, nullptr);
        if (!serverData)
        {
            isNewServer = true;
            serverData = QSharedPointer<ServerData>::create();
            serverData->port = port;
            serverData->name = name;
            _servers[serverId] = serverData;
        }

        bool isNewAddress = false;
        if (isNewServer || !serverData->addresses.contains(address))
        {
            isNewAddress = !isNewServer;
            serverData->addresses.append(address);
        }

        if (isNewServer)
        {
            Q_EMIT foundServer(address, port, serverId, name);
        }
        else if (isNewAddress)
        {
            Q_EMIT foundExtraServerAddress(address, serverId);
        }
    }

    bool ServerDiscoverer::isLocalhostAddress(const QHostAddress& address)
    {
        if (address == QHostAddress::LocalHost || address == QHostAddress::LocalHostIPv6)
            return true;

        for (auto const& localAddress : qAsConst(_localHostNetworkAddresses))
        {
            if (localAddress.isEqual(address, QHostAddress::TolerantConversion))
               return true;
        }

        return false;
    }

    // ============================================================================== //

    ServerProbe::ServerProbe(QObject* parent, QHostAddress const& address, quint16 port)
     : QObject(parent),
       _address(address),
       _port(port),
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

    void ServerProbe::onConnected()
    {
        _connection->sendServerInstanceIdentifierRequest();
        _connection->sendServerNameRequest();
    }

    void ServerProbe::onReceivedServerUuid(QUuid uuid)
    {
        _serverId = uuid;
        emitSignalIfDataComplete();
    }

    void ServerProbe::onReceivedServerName(quint8 nameType, QString name)
    {
        if (_serverNameType > nameType || _serverName == name || name == "")
            return;

        _serverNameType = nameType;
        _serverName = name;

        emitSignalIfDataComplete();
    }

    void ServerProbe::onTimeout()
    {
        if (_connection == nullptr)
            return; /* already cleaned up */

        qDebug() << "ServerProbe: TIMEOUT for" << _address << "port" << _port;

        cleanUpConnection();

        if (!_serverId.isNull()) /* server found but did not receive a name? */
        {
            /* send with empty name */
            Q_EMIT foundServer(_address, _port, _serverId, _serverName);
        }

        deleteLater();
    }

    void ServerProbe::emitSignalIfDataComplete()
    {
        if (_serverId.isNull() || _serverName == "")
            return; /* not yet complete */

        cleanUpConnection();

        Q_EMIT foundServer(_address, _port, _serverId, _serverName);

        deleteLater();
    }

    void ServerProbe::cleanUpConnection()
    {
        _connection->disconnect();
        _connection->deleteLater();
        _connection = nullptr;
    }
}
