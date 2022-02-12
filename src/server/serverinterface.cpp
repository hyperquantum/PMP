/*
    Copyright (C) 2020-2022, Kevin Andre <hyperquantum@gmail.com>

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

#include "delayedstart.h"
#include "generator.h"
#include "player.h"
#include "playerqueue.h"
#include "queueentry.h"
#include "resolver.h"
#include "server.h"
#include "serversettings.h"

#include <QtDebug>

namespace PMP
{
    ServerInterface::ServerInterface(ServerSettings* serverSettings, Server* server,
                                     Player* player, Generator* generator,
                                     DelayedStart* delayedStart)
     : _userLoggedIn(0),
       _serverSettings(serverSettings),
       _server(server),
       _player(player),
       _generator(generator),
       _delayedStart(delayedStart)
    {
        connect(
            _server, &Server::captionChanged,
            this, &ServerInterface::serverCaptionChanged
        );
        connect(
            _server, &Server::serverClockTimeSendingPulse,
            this, &ServerInterface::serverClockTimeSendingPulse
        );
        connect(
            _server, &Server::shuttingDown,
            this, &ServerInterface::serverShuttingDown
        );

        connect(
            _delayedStart, &DelayedStart::delayedStartActiveChanged,
            this, &ServerInterface::delayedStartActiveChanged
        );

        auto* queue = &_player->queue();
        connect(
            queue, &PlayerQueue::entryAdded,
            this, &ServerInterface::onQueueEntryAdded
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

        auto* resolver = &player->resolver();

        connect(
            resolver, &Resolver::fullIndexationRunStatusChanged,
            this, &ServerInterface::fullIndexationRunStatusChanged
        );
    }

    ServerInterface::~ServerInterface()
    {
        qDebug() << "ServerInterface destructor called";
    }

    QUuid ServerInterface::getServerUuid() const
    {
        return _server->uuid();
    }

    QString ServerInterface::getServerCaption() const
    {
        return _server->caption();
    }

    void ServerInterface::setLoggedIn(quint32 userId, QString userLogin)
    {
        _userLoggedIn = userId;
        _userLoggedInName = userLogin;
    }

    void ServerInterface::reloadServerSettings(uint clientReference)
    {
        ResultMessageErrorCode errorCode;

        // TODO : in the future allow reloading if the database is not connected yet
        if (!isLoggedIn())
        {
            errorCode = ResultMessageErrorCode::NotLoggedIn;
        }
        else
        {
            _serverSettings->load();
            errorCode = ResultMessageErrorCode::NoError;
        }

        Q_EMIT serverSettingsReloadResultEvent(clientReference, errorCode);
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

    Result ServerInterface::activateDelayedStart(qint64 delayMilliseconds)
    {
        if (!isLoggedIn())
            return Error::notLoggedIn();

        return _delayedStart->activate(delayMilliseconds);
    }

    Result ServerInterface::deactivateDelayedStart()
    {
        if (!isLoggedIn())
            return Error::notLoggedIn();

        return _delayedStart->deactivate();
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

    PlayerStateOverview ServerInterface::getPlayerStateOverview()
    {
        QueueEntry const* nowPlaying = _player->nowPlaying();

        PlayerStateOverview overview;
        overview.playerState = _player->state();
        overview.nowPlayingQueueId = nowPlaying ? nowPlaying->queueID() : 0;
        overview.trackPosition = _player->playPosition();
        overview.volume = _player->volume();
        overview.queueLength = _player->queue().length();
        overview.delayedStartActive = _delayedStart->isActive();

        return overview;
    }

    Result ServerInterface::enqueue(FileHash hash)
    {
        if (!isLoggedIn())
            return Error::notLoggedIn();

        auto& queue = _player->queue();

        return queue.enqueue(hash);
    }

    Result ServerInterface::insertAtFront(FileHash hash)
    {
        if (!isLoggedIn())
            return Error::notLoggedIn();

        auto& queue = _player->queue();

        return queue.insertAtFront(hash);
    }

    Result ServerInterface::insertBreakAtFrontIfNotExists()
    {
        if (!isLoggedIn())
            return Error::notLoggedIn();

        auto& queue = _player->queue();
        auto* firstEntry = queue.peek();
        if (firstEntry && firstEntry->kind() == QueueEntryKind::Break)
            return Success(); /* already present, nothing to do */

        return queue.insertBreakAtFront();
    }

    Result ServerInterface::insertSpecialQueueItem(SpecialQueueItemType itemType,
                                                   QueueIndexType indexType, int index,
                                                   quint32 clientReference)
    {
        if (!isLoggedIn())
            return Error::notLoggedIn();

        auto& queue = _player->queue();
        index = toNormalIndex(queue, indexType, index);
        if (index < 0)
            return Error::queueIndexOutOfRange();

        return queue.insertAtIndex(index, itemType,
                                   createQueueInsertionIdNotifier(clientReference));
    }

    Result ServerInterface::duplicateQueueEntry(uint id, quint32 clientReference)
    {
        if (!isLoggedIn())
            return Error::notLoggedIn();

        auto& queue = _player->queue();

        auto index = queue.findIndex(id);
        if (index < 0)
            return Error::queueEntryIdNotFound(id);

        auto existing = queue.entryAtIndex(index);
        if (!existing || existing->queueID() != id)
        {
            qWarning() << "queue inconsistency for QID" << id;
            return Error::internalError();
        }

        auto entryCreator = QueueEntryCreators::copyOf(existing);

        return insertAtIndex(index + 1, entryCreator, clientReference);
    }

    Result ServerInterface::insertAtIndex(qint32 index,
                                      std::function<QueueEntry* (uint)> queueEntryCreator,
                                          quint32 clientReference)
    {
        if (!isLoggedIn())
            return Error::notLoggedIn();

        auto& queue = _player->queue();

        return queue.insertAtIndex(index, queueEntryCreator,
                                   createQueueInsertionIdNotifier(clientReference));
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

    bool ServerInterface::isFullIndexationRunning()
    {
        return _player->resolver().fullIndexationRunning();
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

    void ServerInterface::onQueueEntryAdded(qint32 offset, quint32 queueId)
    {
        auto it = _queueEntryInsertionsPending.find(queueId);
        if (it == _queueEntryInsertionsPending.end())
        {
            Q_EMIT queueEntryAddedWithoutReference(offset, queueId);
            return;
        }

        auto clientReference = it.value();
        _queueEntryInsertionsPending.erase(it);

        Q_EMIT queueEntryAddedWithReference(offset, queueId, clientReference);
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

    int ServerInterface::toNormalIndex(const PlayerQueue& queue,
                                       QueueIndexType indexType, int index)
    {
        if (index < 0) /* invalid index */
            return -1;

        if (indexType == QueueIndexType::Normal)
        {
            return index;
        }
        else /* reverse index */
        {
            return queue.length() - index;
        }
    }

    std::function<void (uint)> ServerInterface::createQueueInsertionIdNotifier(
                                                                  quint32 clientReference)
    {
        return
            [this, clientReference](uint queueId)
            {
                _queueEntryInsertionsPending[queueId] = clientReference;
            };
    }
}
