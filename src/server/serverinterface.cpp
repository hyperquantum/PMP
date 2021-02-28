/*
    Copyright (C) 2020-2021, Kevin Andre <hyperquantum@gmail.com>

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

    ServerInterface::ServerInterface(Server* server, uint connectionReference,
                                     Player* player, Generator* generator)
     : _connectionReference(connectionReference), _userLoggedIn(0), _server(server),
       _player(player), _generator(generator)
    {
        connect(
            _server, &Server::shuttingDown,
            this, &ServerInterface::serverShuttingDown
        );

        connect(
            _generator, &Generator::enabledChanged,
            this, &ServerInterface::onDynamicModeStatusChanged
        );
        connect(
            _generator, &Generator::noRepetitionSpanChanged,
            this, &ServerInterface::onDynamicModeNoRepetitionSpanChanged
        );
        connect(
            _generator, &Generator::waveStarting,
            this, &ServerInterface::onDynamicModeWaveStarted
        );
        connect(
            _generator, &Generator::waveProgressChanged,
            this, &ServerInterface::onDynamicModeWaveProgress
        );
        connect(
            _generator, &Generator::waveFinished,
            this, &ServerInterface::onDynamicModeWaveEnded
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

        qDebug() << "ServerInterface: switching to personal mode for user "
                 << _userLoggedInName;

        _player->setUserPlayingFor(_userLoggedIn);
    }

    void ServerInterface::switchToPublicMode()
    {
        if (!isLoggedIn()) return;

        qDebug() << "ServerInterface: switching to public mode";

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
        if (hash.isNull()) return;

        auto& queue = _player->queue();

        if (!queue.canAddMoreEntries())
            return;

        queue.enqueue(hash);
    }

    void ServerInterface::insertAtFront(FileHash hash)
    {
        if (!isLoggedIn()) return;
        if (hash.isNull()) return;

        auto& queue = _player->queue();

        if (!queue.canAddMoreEntries())
            return;

        queue.insertAtFront(hash);
    }

    void ServerInterface::insertBreakAtFront()
    {
        if (!isLoggedIn()) return;

        auto& queue = _player->queue();

        // TODO : if a break is already present, there is no need to bail out here
        if (!queue.canAddMoreEntries())
            return;

        _player->queue().insertBreakAtFront();
    }

    void ServerInterface::insertAtIndex(quint32 index, QueueEntry* entry)
    {
        if (!isLoggedIn()) {
            entry->deleteLater();
            return;
        }

        auto& queue = _player->queue();

        if (!queue.canAddMoreEntries())
            return;

        queue.insertAtIndex(index, entry);
    }

    void ServerInterface::moveQueueEntry(uint id, int upDownOffset)
    {
        if (!isLoggedIn()) return;
        if (id == 0) return;

        _player->queue().moveById(id, upDownOffset);
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

    void ServerInterface::requestDynamicModeStatus()
    {
        auto enabledStatus =
                Common::createUnchangedStartStopEventStatus(_generator->enabled());

        auto noRepetitionSpanSeconds = _generator->noRepetitionSpanSeconds();

        auto user = _generator->userPlayingFor();

        auto waveStatus =
                Common::createUnchangedStartStopEventStatus(_generator->waveActive());

        auto waveProgress = _generator->waveProgress();
        auto waveProgressTotal = _generator->waveProgressTotal();

        Q_EMIT dynamicModeStatusEvent(enabledStatus, noRepetitionSpanSeconds);

        Q_EMIT dynamicModeWaveStatusEvent(waveStatus, user,
                                          waveProgress, waveProgressTotal);
    }

    void ServerInterface::enableDynamicMode()
    {
        if (!isLoggedIn()) return;

        qDebug() << "ServerInterface: enabling dynamic mode";

        _generator->enable();
    }

    void ServerInterface::disableDynamicMode()
    {
        if (!isLoggedIn()) return;

        qDebug() << "ServerInterface: disabling dynamic mode";

        _generator->disable();
    }

    void ServerInterface::startDynamicModeWave()
    {
        if (!isLoggedIn()) return;
        if (_generator->waveActive()) return;

        qDebug() << "ServerInterface: starting dynamic mode wave";

        _generator->startWave();
    }

    void ServerInterface::terminateDynamicModeWave()
    {
        if (!isLoggedIn()) return;
        if (!_generator->waveActive()) return;

        qDebug() << "ServerInterface: terminating dynamic mode wave";

        _generator->terminateWave();
    }

    void ServerInterface::setTrackRepetitionAvoidanceSeconds(int seconds)
    {
        if (!isLoggedIn()) return;
        if (seconds < 0) return;

        qDebug() << "ServerInterface: changing track repetition avoidance interval to"
                 << seconds << "seconds";

        _generator->setNoRepetitionSpanSeconds(seconds);
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

    void ServerInterface::onDynamicModeStatusChanged()
    {
        auto enabledStatus =
                Common::createChangedStartStopEventStatus(_generator->enabled());

        auto noRepetitionSpanSeconds = _generator->noRepetitionSpanSeconds();

        Q_EMIT dynamicModeStatusEvent(enabledStatus, noRepetitionSpanSeconds);
    }

    void ServerInterface::onDynamicModeNoRepetitionSpanChanged()
    {
        auto enabledStatus =
                Common::createUnchangedStartStopEventStatus(_generator->enabled());

        auto noRepetitionSpanSeconds = _generator->noRepetitionSpanSeconds();

        Q_EMIT dynamicModeStatusEvent(enabledStatus, noRepetitionSpanSeconds);
    }

    void ServerInterface::onDynamicModeWaveStarted()
    {
        auto user = _generator->userPlayingFor();

        auto waveStatus =
                Common::createChangedStartStopEventStatus(_generator->waveActive());

        auto waveProgress = _generator->waveProgress();
        auto waveProgressTotal = _generator->waveProgressTotal();

        Q_EMIT dynamicModeWaveStatusEvent(waveStatus, user,
                                          waveProgress, waveProgressTotal);
    }

    void ServerInterface::onDynamicModeWaveProgress(int tracksDelivered, int tracksTotal)
    {
        auto user = _generator->userPlayingFor();

        auto waveStatus =
                Common::createUnchangedStartStopEventStatus(_generator->waveActive());

        Q_EMIT dynamicModeWaveStatusEvent(waveStatus, user,
                                          tracksDelivered, tracksTotal);
    }

    void ServerInterface::onDynamicModeWaveEnded()
    {
        auto user = _generator->userPlayingFor();

        auto waveStatus =
                Common::createChangedStartStopEventStatus(_generator->waveActive());

        auto waveProgress = _generator->waveProgress();
        auto waveProgressTotal = _generator->waveProgressTotal();

        Q_EMIT dynamicModeWaveStatusEvent(waveStatus, user,
                                          waveProgress, waveProgressTotal);
    }

}
