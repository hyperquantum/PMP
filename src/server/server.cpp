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

#include "server.h"

#include "common/networkutil.h"

#include "connectedclient.h"
#include "serverinterface.h"
#include "serversettings.h"

#include <QByteArray>
#include <QHostInfo>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QUdpSocket>

#define QT_USE_QSTRINGBUILDER

namespace PMP
{
    Server::Server(QObject* parent, ServerSettings* serverSettings,
                   const QUuid& serverInstanceIdentifier)
     : QObject(parent),
       _uuid(serverInstanceIdentifier),
       _settings(serverSettings),
       _player(nullptr), _generator(nullptr), _history(nullptr), _users(nullptr),
       _collectionMonitor(nullptr), _serverHealthMonitor(nullptr),
       _server(new QTcpServer(this)), _udpSocket(new QUdpSocket(this)),
       _broadcastTimer(new QTimer(this))
    {
        /* generate a new UUID for ourselves if we did not receive a valid one */
        if (_uuid.isNull()) _uuid = QUuid::createUuid();

        connect(
            _settings, &ServerSettings::serverCaptionChanged,
            this, [this]() { determineCaption(); }
        );
        determineCaption();

        auto fixedServerPassword = serverSettings->fixedServerPassword();
        if (!fixedServerPassword.isEmpty())
        {
            _serverPassword = fixedServerPassword;
        }
        else
        {
            _serverPassword = generateServerPassword();
        }

        connect(
            _server, &QTcpServer::newConnection,
            this, &Server::newConnectionReceived
        );

        connect(_udpSocket, &QUdpSocket::readyRead, this, &Server::readPendingDatagrams);

        connect(_broadcastTimer, &QTimer::timeout, this, &Server::sendBroadcast);

        auto* serverClockTimePulseTimer = new QTimer(this);
        connect(
            serverClockTimePulseTimer, &QTimer::timeout,
            this, &Server::serverClockTimeSendingPulse
        );
        serverClockTimePulseTimer->start(60 * 60 * 1000); /* hourly */
    }

    QString Server::generateServerPassword()
    {
        const QString chars =
            "ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnpqrstuvwxyz123456789!@#%&*()+=:<>?/-";

        const int passwordLength = 8;
        const int consecutiveCharsDistance = 10;

        QString serverPassword;
        serverPassword.reserve(passwordLength);
        int prevIndex = -consecutiveCharsDistance;
        for (int i = 0; i < passwordLength; ++i)
        {
            int index;
            do
            {
                index = qrand() % chars.length(); // FIXME : don't use qrand()
            } while (qAbs(index - prevIndex) < consecutiveCharsDistance);
            prevIndex = index;
            QChar c = chars[index];
            serverPassword += c;
        }

        return serverPassword;
    }

    bool Server::listen(Player* player, Generator* generator, History* history,
                        Users* users,
                        CollectionMonitor* collectionMonitor,
                        ServerHealthMonitor* serverHealthMonitor,
                        const QHostAddress& address, quint16 port)
    {
        _player = player;
        _generator = generator;
        _history = history;
        _users = users;
        _collectionMonitor = collectionMonitor;
        _serverHealthMonitor = serverHealthMonitor;

        if (!_server->listen(address, port))
            return false;

        bool bound =
            _udpSocket->bind(
                23432, QAbstractSocket::ShareAddress | QAbstractSocket::ReuseAddressHint
            );
        if (!bound)
        {
            qWarning() << "UdpSocket bind failed; cannot listen for probes";
        }

        sendBroadcast();
        //_broadcastTimer->start(5000);

        return true;
    }

    QString Server::errorString() const
    {
        return _server->errorString();
    }

    quint16 Server::port() const
    {
        return _server->serverPort();
    }

    void Server::shutdown()
    {
        _broadcastTimer->stop();
        Q_EMIT shuttingDown();
    }

    void Server::newConnectionReceived()
    {
        QTcpSocket *connection = _server->nextPendingConnection();

        auto serverInterface = new ServerInterface(_settings, this, _player, _generator);

        auto connectedClient =
            new ConnectedClient(
                connection, serverInterface, _player, _history, _users,
                _collectionMonitor, _serverHealthMonitor
            );

        connectedClient->setParent(this);
    }

    void Server::sendBroadcast()
    {
        QByteArray datagram = "PMPSERVERANNOUNCEv01 ";
        NetworkUtil::append2Bytes(datagram, port());

        _udpSocket->writeDatagram(datagram, QHostAddress::Broadcast, 23433);
        _udpSocket->flush();
    }

    void Server::readPendingDatagrams()
    {
        while (_udpSocket->hasPendingDatagrams())
        {
            QByteArray datagram;
            datagram.resize(_udpSocket->pendingDatagramSize());
            QHostAddress sender;
            quint16 senderPort;

            _udpSocket->readDatagram(
                datagram.data(), datagram.size(), &sender, &senderPort
            );

            if (datagram.size() < 11 || !datagram.startsWith("PMPPROBEv01"))
                continue;

            qDebug() << "received probe from client port" << senderPort;

            sendBroadcast();
        }
    }

    void Server::determineCaption()
    {
        QString caption = _settings->serverCaption();

        if (caption.isEmpty())
        {
            caption = QHostInfo::localHostName();
        }

        if (caption == _caption)
            return; /* no change */

        _caption = caption;
        Q_EMIT captionChanged();
    }
}
