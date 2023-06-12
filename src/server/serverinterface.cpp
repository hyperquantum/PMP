/*
    Copyright (C) 2020-2023, Kevin Andre <hyperquantum@gmail.com>

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

#include "common/concurrent.h"
#include "common/containerutil.h"
#include "common/promise.h"
#include "common/version.h"

#include "database.h"
#include "delayedstart.h"
#include "generator.h"
#include "hashidregistrar.h"
#include "history.h"
#include "player.h"
#include "playerqueue.h"
#include "queueentry.h"
#include "resolver.h"
#include "scrobbling.h"
#include "serversettings.h"
#include "tcpserver.h"
#include "users.h"

#include <QtDebug>

namespace PMP::Server
{
    ServerInterface::ServerInterface(ServerSettings* serverSettings, TcpServer* server,
                                     uint connectionReference,
                                     Player* player, Generator* generator,
                                     History* history,
                                     HashIdRegistrar* hashIdRegistrar,
                                     Users* users,
                                     DelayedStart* delayedStart, Scrobbling* scrobbling)
     : _connectionReference(connectionReference),
       _userLoggedIn(0),
       _serverSettings(serverSettings),
       _server(server),
       _player(player),
       _generator(generator),
       _history(history),
       _hashIdRegistrar(hashIdRegistrar),
       _users(users),
       _delayedStart(delayedStart),
       _scrobbling(scrobbling)
    {
        connect(
            _server, &TcpServer::captionChanged,
            this, &ServerInterface::serverCaptionChanged
        );
        connect(
            _server, &TcpServer::serverClockTimeSendingPulse,
            this, &ServerInterface::serverClockTimeSendingPulse
        );
        connect(
            _server, &TcpServer::shuttingDown,
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

        connect(
            _history, &History::hashStatisticsChanged,
            this, &ServerInterface::onHashStatisticsChanged
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

    VersionInfo ServerInterface::getServerVersionInfo() const
    {
        VersionInfo info;
        info.programName = PMP_PRODUCT_NAME;
        info.versionForDisplay = PMP_VERSION_DISPLAY;
        info.vcsBuild = VCS_REVISION_LONG;
        info.vcsBranch = VCS_BRANCH;

        return info;
    }

    ResultOrError<QUuid, Result> ServerInterface::getDatabaseUuid() const
    {
        auto uuid = Database::getDatabaseUuid();

        if (uuid.isNull())
            return Error::internalError();

        return uuid;
    }

    void ServerInterface::setLoggedIn(quint32 userId, QString userLogin)
    {
        _userLoggedIn = userId;
        _userLoggedInName = userLogin;
    }

    SimpleFuture<ResultMessageErrorCode> ServerInterface::reloadServerSettings()
    {
        SimplePromise<ResultMessageErrorCode> promise;

        // TODO : in the future allow reloading if the database is not connected yet
        if (!isLoggedIn())
        {
            promise.setResult(ResultMessageErrorCode::NotLoggedIn);
        }
        else
        {
            _serverSettings->load();
            promise.setResult(ResultMessageErrorCode::NoError);
        }

        return promise.future();
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

    void ServerInterface::requestScrobblingInfo()
    {
        if (!isLoggedIn()) return;

        auto controller = _scrobbling->getControllerForUser(_userLoggedIn);

        controller->requestScrobblingProviderInfo();
    }

    void ServerInterface::setScrobblingProviderEnabled(ScrobblingProvider provider,
                                                       bool enabled)
    {
        if (!isLoggedIn()) return;

        if (provider == ScrobblingProvider::Unknown)
            return; /* provider invalid or not recognized */

        auto controller = _scrobbling->getControllerForUser(_userLoggedIn);

        controller->setScrobblingProviderEnabled(provider, enabled);
    }

    SimpleFuture<Result> ServerInterface::authenticateScrobblingProvider(
                                                            ScrobblingProvider provider,
                                                            QString user,
                                                            QString password)
    {
        if (!isLoggedIn())
            return FutureResult(Error::notLoggedIn());

        if (provider == ScrobblingProvider::Unknown)
            return FutureResult(Error::scrobblingProviderInvalid());

        return _scrobbling->authenticateForProvider(_userLoggedIn, provider, user,
                                                    password);
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

    bool ServerInterface::delayedStartActive() const
    {
        return _delayedStart->isActive();
    }

    qint64 ServerInterface::getDelayedStartTimeRemainingMilliseconds() const
    {
        return _delayedStart->timeRemainingMilliseconds();
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
        auto nowPlaying = _player->nowPlaying();

        PlayerStateOverview overview;
        overview.playerState = _player->state();
        overview.nowPlayingQueueId = nowPlaying ? nowPlaying->queueID() : 0;
        overview.trackPosition = _player->playPosition();
        overview.volume = _player->volume();
        overview.queueLength = _player->queue().length();
        overview.delayedStartActive = _delayedStart->isActive();

        return overview;
    }

    Future<QVector<QString>, Result> ServerInterface::getPossibleFilenamesForQueueEntry(
                                                                                  uint id)
    {
        if (id <= 0) /* invalid queue ID */
            return FutureError(Error::queueEntryIdNotFound(0));

        auto entry = _player->queue().lookup(id);
        if (entry == nullptr) /* ID not found */
            return FutureError(Error::queueEntryIdNotFound(id));

        if (!entry->isTrack())
            return FutureError(Error::queueItemTypeInvalid());

        auto hash = entry->hash().value();
        uint hashId = _player->resolver().getID(hash);

        auto future =
            Concurrent::run<QVector<QString>, FailureType>(
                [hashId]() -> ResultOrError<QVector<QString>, FailureType>
                {
                    auto db = Database::getDatabaseForCurrentThread();
                    if (!db)
                        return failure; /* database unusable */

                    return db->getFilenames(hashId);
                }
            )
            .convertError<Result>([](FailureType) { return Error::internalError(); });

        return future;
    }

    Result ServerInterface::insertTrackAtEnd(FileHash hash)
    {
        if (!isLoggedIn())
            return Error::notLoggedIn();

        if (_hashIdRegistrar->isRegistered(hash) == false)
            return Error::hashIsUnknown();

        auto& queue = _player->queue();

        return queue.enqueue(hash);
    }

    Result ServerInterface::insertTrackAtFront(FileHash hash)
    {
        if (!isLoggedIn())
            return Error::notLoggedIn();

        if (_hashIdRegistrar->isRegistered(hash) == false)
            return Error::hashIsUnknown();

        auto& queue = _player->queue();

        return queue.insertAtFront(hash);
    }

    Result ServerInterface::insertBreakAtFrontIfNotExists()
    {
        if (!isLoggedIn())
            return Error::notLoggedIn();

        auto& queue = _player->queue();
        auto firstEntry = queue.peek();
        if (firstEntry && firstEntry->kind() == QueueEntryKind::Break)
            return Success(); /* already present, nothing to do */

        return queue.insertBreakAtFront();
    }

    Result ServerInterface::insertTrack(FileHash hash, int index, quint32 clientReference)
    {
        if (!isLoggedIn())
            return Error::notLoggedIn();

        if (_hashIdRegistrar->isRegistered(hash) == false)
            return Error::hashIsUnknown();

        auto entryCreator = QueueEntryCreators::hash(hash);

        return insertAtIndex(index, entryCreator, clientReference);
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
                       std::function<QSharedPointer<QueueEntry> (uint)> queueEntryCreator,
                       quint32 clientReference)
    {
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

    void ServerInterface::requestHashUserData(quint32 userId, QVector<FileHash> hashes)
    {
        if (!isLoggedIn()) return;

        if (userId != 0 && !_users->checkUserIdExists(userId))
            return;

        /* we make sure not to trigger registration of unknown hashes */
        const auto existingHashes = _hashIdRegistrar->getExistingIdsOnly(hashes);

        QVector<HashStats> hashStatsAlreadyAvailable;
        hashStatsAlreadyAvailable.reserve(existingHashes.size());

        for (auto const& idAndHash : existingHashes)
        {
            auto statsOrNull = _history->getUserStats(idAndHash.first, userId);
            if (statsOrNull == null)
                continue; /* stats will arrive after a delay */

            HashStats stats(idAndHash.second, statsOrNull.value());
            hashStatsAlreadyAvailable.append(stats);
        }

        /* if possible, reply immediately with the information that is already known */
        if (!hashStatsAlreadyAvailable.isEmpty())
            Q_EMIT hashUserDataChangedOrAvailable(userId, hashStatsAlreadyAvailable);
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

    void ServerInterface::onHashStatisticsChanged(quint32 userId, QVector<uint> hashIds)
    {
        addUserHashDataNotification(userId, hashIds);
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

    void ServerInterface::addUserHashDataNotification(quint32 userId,
                                                      QVector<uint> hashIds)
    {
        ContainerUtil::addToSet(hashIds, _userHashDataNotificationsPending[userId]);

        if (_userHashDataNotificationTimerRunning.value(userId, false) == false)
        {
            _userHashDataNotificationTimerRunning[userId] = true;
            QTimer::singleShot(
                100,
                this,
                [this, userId]()
                {
                    _userHashDataNotificationTimerRunning[userId] = false;
                    sendUserHashDataNotifications(userId);
                }
            );
        }
    }

    void ServerInterface::sendUserHashDataNotifications(quint32 userId)
    {
        QVector<HashStats> statsToSend;

        {
            auto const& hashesPendingForUser = _userHashDataNotificationsPending[userId];

            if (hashesPendingForUser.isEmpty())
                return;

            statsToSend.reserve(hashesPendingForUser.size());

            for (auto hashId : hashesPendingForUser)
            {
                auto hashOrNull = _hashIdRegistrar->getHashForId(hashId);
                if (hashOrNull == null)
                {
                    qWarning() << "ServerInterface: could not get hash for hash ID"
                               << hashId;
                    continue;
                }

                auto statsOrNull = _history->getUserStats(hashId, userId);
                if (statsOrNull == null)
                {
                    qWarning() << "ServerInterface: stats have disappeared for hash ID"
                               << hashId << "and user ID" << userId;
                    continue;
                }

                statsToSend.append(HashStats(hashOrNull.value(), statsOrNull.value()));
            }
        }

        _userHashDataNotificationsPending.remove(userId);

        if (!statsToSend.isEmpty())
            Q_EMIT hashUserDataChangedOrAvailable(userId, statsToSend);
    }
}
