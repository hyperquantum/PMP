/*
    Copyright (C) 2020, Kevin Andre <hyperquantum@gmail.com>

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

#include "serverinterface.h"

#include "generator.h"
#include "player.h"
#include "playerqueue.h"
#include "queueentry.h"
#include "resolver.h"
#include "server.h"

#include <QtDebug>

namespace PMP {

    ServerInterface::ServerInterface(Server* server, Player* player, Generator* generator)
     : _userLoggedIn(0), _server(server), _player(player), _generator(generator)
    {
        connect(
            _server, &Server::shuttingDown,
            this, &ServerInterface::serverShuttingDown
        );
    }

    QUuid ServerInterface::getServerUuid() const
    {
        return _server->uuid();
    }

    void ServerInterface::setLoggedIn(quint32 userId, QString userLogin)
    {
        _userLoggedIn = userId;
        _userLoggedInName = userLogin;
    }

    void ServerInterface::switchToPersonalMode()
    {
        if (!isLoggedIn()) return;

        qDebug() << "switching to personal mode for user " << _userLoggedInName;
        _player->setUserPlayingFor(_userLoggedIn);
    }

    void ServerInterface::switchToPublicMode()
    {
        if (!isLoggedIn()) return;

        _player->setUserPlayingFor(0);
    }

    void ServerInterface::play()
    {
        if (!isLoggedIn()) return;
        _player->play();
    }

    void ServerInterface::pause()
    {
        if (!isLoggedIn()) return;
        _player->pause();
    }

    void ServerInterface::skip()
    {
        if (!isLoggedIn()) return;
        _player->skip();
    }

    void ServerInterface::seekTo(qint64 positionMilliseconds)
    {
        if (!isLoggedIn()) return;
        if (positionMilliseconds < 0) return; /* invalid position */

        _player->seekTo(positionMilliseconds);
    }

    void ServerInterface::setVolume(int volumePercentage)
    {
        if (!isLoggedIn()) return;
        if (volumePercentage < 0 || volumePercentage > 100) return;

        _player->setVolume(volumePercentage);
    }

    void ServerInterface::enqueue(FileHash hash)
    {
        if (!isLoggedIn()) return;
        if (hash.empty()) return;

        _player->queue().enqueue(hash);
    }

    void ServerInterface::insertAtFront(FileHash hash)
    {
        if (!isLoggedIn()) return;
        if (hash.empty()) return;

        _player->queue().insertAtFront(hash);
    }

    void ServerInterface::insertBreakAtFront()
    {
        if (!isLoggedIn()) return;
        _player->queue().insertBreakAtFront();
    }

    void ServerInterface::insertAtIndex(quint32 index, QueueEntry* entry)
    {
        if (!isLoggedIn()) {
            entry->deleteLater();
            return;
        }

        _player->queue().insertAtIndex(index, entry);
    }

    void ServerInterface::moveQueueEntry(uint id, int upDownOffset)
    {
        if (!isLoggedIn()) return;
        if (id == 0) return;

        _player->queue().move(id, upDownOffset);
    }

    void ServerInterface::removeQueueEntry(uint id)
    {
        if (!isLoggedIn()) return;
        if (id == 0) return;

        _player->queue().remove(id);
    }

    void ServerInterface::trimQueue()
    {
        if (!isLoggedIn()) return;

        /* TODO: get the '10' from elsewhere */
       _player->queue().trim(10);
    }

    void ServerInterface::requestQueueExpansion()
    {
        if (!isLoggedIn()) return;
        _generator->requestQueueExpansion();
    }

    void ServerInterface::enableDynamicMode()
    {
        if (!isLoggedIn()) return;
        _generator->enable();
    }

    void ServerInterface::disableDynamicMode()
    {
        if (!isLoggedIn()) return;
        _generator->disable();
    }

    void ServerInterface::startWave()
    {
        if (!isLoggedIn()) return;
        if (_generator->waveActive()) return;

        _generator->startWave();
    }

    void ServerInterface::setTrackRepetitionAvoidanceMinutes(int minutes)
    {
        if (!isLoggedIn()) return;
        if (minutes < 0) return;

        _generator->setNoRepetitionSpan(minutes);
    }

    void ServerInterface::startFullIndexation()
    {
        if (!isLoggedIn()) return;
        _player->resolver().startFullIndexation();
    }

    void ServerInterface::shutDownServer()
    {
        if (!isLoggedIn()) return;
        _server->shutdown();
    }

    void ServerInterface::shutDownServer(QString serverPassword)
    {
        if (serverPassword != _server->serverPassword()) return;
        _server->shutdown();
    }

}
