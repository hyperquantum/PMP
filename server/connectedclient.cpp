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

#include "connectedclient.h"

#include "common/hashid.h"

#include "player.h"
#include "queueentry.h"
#include "server.h"

namespace PMP {

    ConnectedClient::ConnectedClient(QTcpSocket* socket, Server* server, Player* player)
     : QObject(server), _socket(socket), _server(server), _player(player)
    {
        connect(server, SIGNAL(shuttingDown()), this, SLOT(terminateConnection()));
        connect(socket, SIGNAL(disconnected()), this, SLOT(terminateConnection()));
        connect(socket, SIGNAL(readyRead()), this, SLOT(dataArrived()));
        connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(socketError(QAbstractSocket::SocketError)));
        connect(player, SIGNAL(volumeChanged(int)), this, SLOT(volumeChanged(int)));
    }


    void ConnectedClient::terminateConnection() {
        _socket->close();
        _readBuffer.clear();
        _socket->deleteLater();
        this->deleteLater();
    }

    void ConnectedClient::dataArrived() {
        _readBuffer.append(_socket->readAll());

        do {
            int semicolonIndex = _readBuffer.indexOf(';');
            if (semicolonIndex < 0) {
                break; // no complete text command in received data
            }

            QString commandString = QString::fromUtf8(_readBuffer.data(), semicolonIndex);

            _readBuffer.remove(0, semicolonIndex + 1); /* +1 to remove the semicolon too */

            executeTextCommand(commandString);
        } while (true);
    }

    void ConnectedClient::socketError(QAbstractSocket::SocketError error) {
        switch (error) {
            case QAbstractSocket::RemoteHostClosedError:
                this->terminateConnection();
                break;
            default:
                // problem ??

                break;
        }
    }

    void ConnectedClient::executeTextCommand(QString const& commandText) {
        int spaceIndex = commandText.indexOf(' ');
        QString command = commandText;

        if (spaceIndex < 0) { /* command without arguments */
            if (command == "play") {
                _player->play();
            }
            else if (command == "pause") {
                _player->pause();
            }
            else if (command == "skip") {
                _player->skip();
            }
            else if (command == "volume") {
                /* 'volume' without arguments sends current volume */
                _socket->write((QString("volume ") + QString::number(_player->volume()) + ";").toUtf8());
            }
            else if (command == "nowplaying") {
                QueueEntry const* now = _player->nowPlaying();
                if (now == 0) {
                    _socket->write(QString("nowplaying nothing;").toUtf8());
                }
                else {
                    int seconds = now->lengthInSeconds();
                    HashID const* hash = now->hash();

                    _socket->write(
                        (QString("nowplaying track \n title: ") + now->title()
                         + "\n artist: " + now->artist()
                         + "\n length: " + (seconds < 0 ? "?" : QString::number(seconds))
                         + " sec\n hash length: " + (hash == 0 ? "?" : QString::number(hash->length()))
                         + "\n hash SHA-1: " + (hash == 0 ? "?" : hash->SHA1().toHex())
                         + "\n hash MD5: " + (hash == 0 ? "?" : hash->MD5().toHex())
                         + ";"
                        ).toUtf8()
                    );
                }
            }
            else if (command == "shutdown") {
                _server->shutdown();
            }
            else {
                /* unknown command ???? */

            }

            return;
        }

        /* split command at the space; don't include the space in the parts */
        QString rest = command.mid(spaceIndex + 1);
        command = command.left(spaceIndex);
        spaceIndex = rest.indexOf(' ');

        // one argument?
        if (spaceIndex < 0) {
            // 'volume' with one argument changes current volume
            if (command == "volume") {
                bool ok;
                uint volume = rest.toUInt(&ok);
                if (ok && volume >= 0 && volume <= 100) {
                    _player->setVolume(volume);
                }
            }
            else {
                /* unknown command ???? */

            }
        }

    }

    void ConnectedClient::volumeChanged(int volume) {
        _socket->write((QString("volume ") + QString::number(_player->volume()) + ";").toUtf8());
    }

}
