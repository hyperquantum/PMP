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

#include "commands.h"

#include "common/util.h"

#include "client/clientserverinterface.h"
#include "client/currenttrackmonitor.h"
#include "client/generalcontroller.h"
#include "client/playercontroller.h"
#include "client/queuecontroller.h"
#include "client/queueentryinfostorage.h"
#include "client/queuemonitor.h"
#include "client/userdatafetcher.h"

namespace PMP
{
    /* ===== ServerVersionCommand ===== */

    bool ServerVersionCommand::requiresAuthentication() const
    {
        return false;
    }

    void ServerVersionCommand::setUp(ClientServerInterface* clientServerInterface)
    {
        Q_UNUSED(clientServerInterface)
    }

    void ServerVersionCommand::start(ClientServerInterface* clientServerInterface)
    {
        auto future = clientServerInterface->generalController().getServerVersionInfo();

        future.addFailureListener(
            this,
            [this](ResultMessageErrorCode errorCode)
            {
                setCommandExecutionResult(errorCode);
            }
        );

        future.addResultListener(
            this,
            [this](VersionInfo versionInfo)
            {
                printVersion(versionInfo);
            }
        );
    }

    void ServerVersionCommand::printVersion(const VersionInfo& versionInfo)
    {
        QString text =
            versionInfo.programName % "\n"
                % "version: " % versionInfo.versionForDisplay % "\n"
                % "build: " % versionInfo.vcsBuild % " - " % versionInfo.vcsBranch;

        setCommandExecutionSuccessful(text);
    }

    /* ===== ReloadServerSettingsCommand ===== */

    ReloadServerSettingsCommand::ReloadServerSettingsCommand()
    {
        //
    }

    bool ReloadServerSettingsCommand::requiresAuthentication() const
    {
        return true;
    }

    void ReloadServerSettingsCommand::setUp(ClientServerInterface* clientServerInterface)
    {
        Q_UNUSED(clientServerInterface)
    }

    void ReloadServerSettingsCommand::start(ClientServerInterface* clientServerInterface)
    {
        auto future = clientServerInterface->generalController().reloadServerSettings();
        addCommandExecutionFutureListener(future);
    }

    /* ===== DelayedStartAtCommand ===== */

    DelayedStartAtCommand::DelayedStartAtCommand(QDateTime startTime)
     : _startTime(startTime)
    {
        //
    }

    bool DelayedStartAtCommand::requiresAuthentication() const
    {
        return true;
    }

    void DelayedStartAtCommand::setUp(ClientServerInterface* clientServerInterface)
    {
        Q_UNUSED(clientServerInterface)
    }

    void DelayedStartAtCommand::start(ClientServerInterface* clientServerInterface)
    {
        auto future =
            clientServerInterface->playerController().activateDelayedStart(_startTime);

        addCommandExecutionFutureListener(future);
    }

    /* ===== DelayedStartWaitCommand ===== */

    DelayedStartWaitCommand::DelayedStartWaitCommand(qint64 delayMilliseconds)
     : _delayMilliseconds(delayMilliseconds)
    {
        //
    }

    bool DelayedStartWaitCommand::requiresAuthentication() const
    {
        return true;
    }

    void DelayedStartWaitCommand::setUp(ClientServerInterface* clientServerInterface)
    {
        Q_UNUSED(clientServerInterface)
    }

    void DelayedStartWaitCommand::start(ClientServerInterface* clientServerInterface)
    {
        auto future =
            clientServerInterface->playerController().activateDelayedStart(
                                                                      _delayMilliseconds);
        addCommandExecutionFutureListener(future);
    }

    /* ===== DeactivateDelayedCancelCommand ===== */

    bool DelayedStartCancelCommand::requiresAuthentication() const
    {
        return true;
    }

    void DelayedStartCancelCommand::setUp(ClientServerInterface* clientServerInterface)
    {
        Q_UNUSED(clientServerInterface)
    }

    void DelayedStartCancelCommand::start(ClientServerInterface* clientServerInterface)
    {
        auto future = clientServerInterface->playerController().deactivateDelayedStart();
        addCommandExecutionFutureListener(future);
    }

    /* ===== PlayCommand ===== */

    PlayCommand::PlayCommand()
    {
        //
    }

    bool PlayCommand::requiresAuthentication() const
    {
        return true;
    }

    void PlayCommand::setUp(ClientServerInterface* clientServerInterface)
    {
        auto* playerController = &clientServerInterface->playerController();

        connect(playerController, &PlayerController::playerStateChanged,
                this, &PlayCommand::listenerSlot);

        addStep(
            [this, playerController]() -> bool
            {
                if (playerController->playerState() == PlayerState::Playing)
                    setCommandExecutionSuccessful();

                return false;
            }
        );
    }

    void PlayCommand::start(ClientServerInterface* clientServerInterface)
    {
        clientServerInterface->playerController().play();
    }

    /* ===== PauseCommand ===== */

    PauseCommand::PauseCommand()
    {
        //
    }

    bool PauseCommand::requiresAuthentication() const
    {
        return true;
    }

    void PauseCommand::setUp(ClientServerInterface* clientServerInterface)
    {
        auto* playerController = &clientServerInterface->playerController();

        connect(playerController, &PlayerController::playerStateChanged,
                this, &PauseCommand::listenerSlot);

        addStep(
            [this, playerController]() -> bool
            {
                if (playerController->playerState() == PlayerState::Paused)
                    setCommandExecutionSuccessful();

                return false;
            }
        );
    }

    void PauseCommand::start(ClientServerInterface* clientServerInterface)
    {
        clientServerInterface->playerController().pause();
    }

    /* ===== SkipCommand ===== */

    SkipCommand::SkipCommand()
     : _currentQueueId(0)
    {
        //
    }

    bool SkipCommand::requiresAuthentication() const
    {
        return true;
    }

    void SkipCommand::setUp(ClientServerInterface* clientServerInterface)
    {
        auto* playerController = &clientServerInterface->playerController();

        connect(playerController, &PlayerController::playerStateChanged,
                this, &SkipCommand::listenerSlot);
        connect(playerController, &PlayerController::currentTrackChanged,
                this, &SkipCommand::listenerSlot);

        addStep(
            [this, playerController]() -> bool
            {
                if (playerController->playerState() == PlayerState::Unknown)
                    return false;

                if (playerController->canSkip())
                {
                    _currentQueueId = playerController->currentQueueId();
                    playerController->skip();
                }
                else
                    setCommandExecutionFailed(3, "player cannot skip now");

                return true;
            }
        );
        addStep(
            [this, playerController]() -> bool
            {
                if (playerController->currentQueueId() != _currentQueueId)
                    setCommandExecutionSuccessful();

                return false;
            }
        );
    }

    void SkipCommand::start(ClientServerInterface* clientServerInterface)
    {
        Q_UNUSED(clientServerInterface)
        // no specific start action needed
    }

    /* ===== NowPlayingCommand ===== */

    NowPlayingCommand::NowPlayingCommand()
    {
        //
    }

    bool NowPlayingCommand::requiresAuthentication() const
    {
        return false;
    }

    void NowPlayingCommand::setUp(ClientServerInterface* clientServerInterface)
    {
        auto* currentTrackMonitor = &clientServerInterface->currentTrackMonitor();

        connect(currentTrackMonitor, &CurrentTrackMonitor::currentTrackChanged,
                this, &NowPlayingCommand::listenerSlot);
        connect(currentTrackMonitor, &CurrentTrackMonitor::currentTrackInfoChanged,
                this, &NowPlayingCommand::listenerSlot);

        addStep(
            [this, currentTrackMonitor]() -> bool
            {
                auto isTrackPresent = currentTrackMonitor->isTrackPresent();

                if (isTrackPresent.isFalse())
                {
                    setCommandExecutionSuccessful("Now playing: nothing");
                    return false;
                }

                if (isTrackPresent.isUnknown()
                        || currentTrackMonitor->currentTrackHash().isNull())
                    return false;

                auto title = currentTrackMonitor->currentTrackTitle();
                auto artist = currentTrackMonitor->currentTrackArtist();
                auto possibleFileName =
                        currentTrackMonitor->currentTrackPossibleFilename();

                if (title.isEmpty() && artist.isEmpty() && possibleFileName.isEmpty())
                    return false;

                auto queueId = currentTrackMonitor->currentQueueId();
                auto lengthMilliseconds =
                        currentTrackMonitor->currentTrackLengthMilliseconds();
                auto lengthString =
                        lengthMilliseconds < 0
                            ? ""
                            : Util::millisecondsToLongDisplayTimeText(lengthMilliseconds);
                auto hashString = currentTrackMonitor->currentTrackHash().toString();

                QString output;
                output.reserve(100);
                output += "Now playing: track\n";
                output += " QID: " + QString::number(queueId) + "\n";
                output += " title: " + title + "\n";
                output += " artist: " + artist + "\n";
                output += " length: " + lengthString + "\n";

                if (title.isEmpty() && artist.isEmpty())
                    output += " possible filename: " + possibleFileName + "\n";

                output += " hash: " + hashString; // no newline at the end here

                setCommandExecutionSuccessful(output);
                return false;
            }
        );
    }

    void NowPlayingCommand::start(ClientServerInterface* clientServerInterface)
    {
        Q_UNUSED(clientServerInterface)
        // no specific start action needed
    }

    /* ===== QueueCommand ===== */

    QueueCommand::QueueCommand()
     : _fetchLimit(10)
    {
        //
    }

    bool QueueCommand::requiresAuthentication() const
    {
        return true;
    }

    void QueueCommand::setUp(ClientServerInterface* clientServerInterface)
    {
        auto* queueMonitor = &clientServerInterface->queueMonitor();
        queueMonitor->setFetchLimit(_fetchLimit);

        auto* queueEntryInfoStorage = &clientServerInterface->queueEntryInfoStorage();

        (void)clientServerInterface->queueEntryInfoFetcher(); /* speed things up */

        connect(queueMonitor, &QueueMonitor::fetchCompleted,
                this, &QueueCommand::listenerSlot);
        connect(queueEntryInfoStorage, &QueueEntryInfoStorage::tracksChanged,
                this, &QueueCommand::listenerSlot);

        addStep(
            [this, queueMonitor, queueEntryInfoStorage]() -> bool
            {
                if (!queueMonitor->isFetchCompleted())
                    return false;

                bool needToWaitForFilename = false;
                for (int i = 0; i < queueMonitor->queueLength() && i < _fetchLimit; ++i)
                {
                    auto queueId = queueMonitor->queueEntry(i);
                    if (queueId == 0)
                        return false; /* download incomplete, shouldn't happen */

                    auto entry = queueEntryInfoStorage->entryInfoByQueueId(queueId);
                    if (!entry || entry->type() == QueueEntryType::Unknown)
                        return false; /* info not available yet */

                    if (entry->needFilename())
                        needToWaitForFilename = true;
                }

                if (needToWaitForFilename)
                    setStepDelay(50);

                return true;
            }
        );
        addStep(
            [this, queueMonitor, queueEntryInfoStorage]() -> bool
            {
                printQueue(queueMonitor, queueEntryInfoStorage);
                return false;
            }
        );
    }

    void QueueCommand::start(ClientServerInterface* clientServerInterface)
    {
        Q_UNUSED(clientServerInterface)
        // no specific start action needed
    }

    void QueueCommand::printQueue(AbstractQueueMonitor* queueMonitor,
                                  QueueEntryInfoStorage* queueEntryInfoStorage)
    {
        QString output;
        output.reserve(80 + 80 + 80 * _fetchLimit);

        output += "queue length " + QString::number(queueMonitor->queueLength()) + "\n";
        output += "Index|  QID  | Length | Title                          | Artist";

        for (int index = 0;
             index < queueMonitor->queueLength() && index < _fetchLimit;
             ++index)
        {
            output += "\n";
            output += QString::number(index).rightJustified(5);
            output += "|";

            auto queueId = queueMonitor->queueEntry(index);
            if (queueId == 0)
            {
                output += "??????????"; /* shouldn't happen */
                continue;
            }

            output += QString::number(queueId).rightJustified(7);
            output += "|";

            auto entry = queueEntryInfoStorage->entryInfoByQueueId(queueId);
            if (!entry)
            {
                output += "??????????"; /* info not available yet, unlikely but possible*/
                continue;
            }

            auto lengthMilliseconds = entry->lengthInMilliseconds();
            if (lengthMilliseconds >= 0)
            {
                auto lengthString =
                        Util::millisecondsToShortDisplayTimeText(lengthMilliseconds);
                output += lengthString.rightJustified(8);
                output += "|";
            }
            else if (entry->isTrack().toBool(true))
            {
                output += "   ??   ";
                output += "|";
            }
            else
            {
                output += "        ";
            }

            if (entry->isTrack().toBool(false) == false)
            {
                output += "      ";
                output += getSpecialEntryText(entry);
            }
            else if (entry->needFilename() && !entry->informativeFilename().isEmpty())
            {
                output += entry->informativeFilename();
            }
            else
            {
                output += entry->title().leftJustified(32);
                output += "|";
                output += entry->artist();
            }
        }

        if (_fetchLimit < queueMonitor->queueLength())
        {
            output += "\n...";
        }

        setCommandExecutionSuccessful(output);
    }

    QString QueueCommand::getSpecialEntryText(const QueueEntryInfo* entry) const
    {
        switch (entry->type())
        {
            case QueueEntryType::Track:
                /* shouldn't happen */
                return "";

            case QueueEntryType::BreakPoint:
                return "----------- BREAK -----------";

            case QueueEntryType::Barrier:
                return "---------- BARRIER ----------";

            case QueueEntryType::UnknownSpecialType:
                return "<<<< UNKNOWN ENTITY >>>>";

            case QueueEntryType::Unknown:
                return "???????????";
        }

        /* shouldn't happen */
        return "";
    }

    /* ===== ShutdownCommand ===== */

    ShutdownCommand::ShutdownCommand(/*QString serverPassword*/)
    // : _serverPassword(serverPassword)
    {
        //
    }

    bool ShutdownCommand::requiresAuthentication() const
    {
        return true;
    }

    bool ShutdownCommand::willCauseDisconnect() const
    {
        return true;
    }

    void ShutdownCommand::setUp(ClientServerInterface* clientServerInterface)
    {
        connect(clientServerInterface, &ClientServerInterface::connectedChanged,
                this, &ShutdownCommand::listenerSlot);

        addStep(
            [this, clientServerInterface]() -> bool
            {
                if (!clientServerInterface->connected())
                    setCommandExecutionSuccessful();

                return false;
            }
        );
    }

    void ShutdownCommand::start(ClientServerInterface* clientServerInterface)
    {
        clientServerInterface->generalController().shutdownServer();
    }

    /* ===== GetVolumeCommand ===== */

    GetVolumeCommand::GetVolumeCommand()
    {
        //
    }

    bool GetVolumeCommand::requiresAuthentication() const
    {
        return false;
    }

    void GetVolumeCommand::setUp(ClientServerInterface* clientServerInterface)
    {
        auto* playerController = &clientServerInterface->playerController();

        connect(playerController, &PlayerController::volumeChanged,
                this, &GetVolumeCommand::listenerSlot);

        addStep(
            [this, playerController]() -> bool
            {
                auto volume = playerController->volume();

                if (volume >= 0)
                    setCommandExecutionSuccessful("Volume: " + QString::number(volume));

                return false;
            }
        );
    }

    void GetVolumeCommand::start(ClientServerInterface* clientServerInterface)
    {
        Q_UNUSED(clientServerInterface)
        // no specific start action needed
    }

    /* ===== SetVolumeCommand ===== */

    SetVolumeCommand::SetVolumeCommand(int volume)
     : _volume(volume)
    {
        //
    }

    bool SetVolumeCommand::requiresAuthentication() const
    {
        return true;
    }

    void SetVolumeCommand::setUp(ClientServerInterface* clientServerInterface)
    {
        auto* playerController = &clientServerInterface->playerController();

        connect(playerController, &PlayerController::volumeChanged,
                this, &SetVolumeCommand::listenerSlot);

        addStep(
            [this, playerController]() -> bool
            {
                if (playerController->volume() == _volume)
                    setCommandExecutionSuccessful();

                return false;
            }
        );
    }

    void SetVolumeCommand::start(ClientServerInterface* clientServerInterface)
    {
        clientServerInterface->playerController().setVolume(_volume);
    }

    /* ===== BreakCommand =====*/

    BreakCommand::BreakCommand()
    {
        //
    }

    bool BreakCommand::requiresAuthentication() const
    {
        return true;
    }

    void BreakCommand::setUp(ClientServerInterface* clientServerInterface)
    {
        auto* queueMonitor = &clientServerInterface->queueMonitor();
        queueMonitor->setFetchLimit(1);

        auto* queueEntryInfoStorage = &clientServerInterface->queueEntryInfoStorage();

        connect(queueMonitor, &QueueMonitor::fetchCompleted,
                this, &BreakCommand::listenerSlot);
        connect(queueEntryInfoStorage, &QueueEntryInfoStorage::tracksChanged,
                this, &BreakCommand::listenerSlot);

        addStep(
            [this, queueMonitor, queueEntryInfoStorage]() -> bool
            {
                if (!queueMonitor->isFetchCompleted())
                    return false;

                if (queueMonitor->queueLength() == 0)
                    return false;

                auto firstEntryId = queueMonitor->queueEntry(0);
                if (firstEntryId == 0)
                    return false; /* shouldn't happen */

                auto firstEntry = queueEntryInfoStorage->entryInfoByQueueId(firstEntryId);
                if (!firstEntry)
                    return false;

                if (firstEntry->type() != QueueEntryType::BreakPoint)
                    return false;

                setCommandExecutionSuccessful();
                return false;
            }
        );
    }

    void BreakCommand::start(ClientServerInterface* clientServerInterface)
    {
        clientServerInterface->queueController().insertBreakAtFrontIfNotExists();
    }

    /* ===== QueueInsertSpecialItemCommand ===== */

    QueueInsertSpecialItemCommand::QueueInsertSpecialItemCommand(
                                                            SpecialQueueItemType itemType,
                                                            int index,
                                                            QueueIndexType indexType)
     : _itemType(itemType),
       _index(index),
       _indexType(indexType)
    {
        //
    }

    bool QueueInsertSpecialItemCommand::requiresAuthentication() const
    {
        return true;
    }

    void QueueInsertSpecialItemCommand::setUp(
                                             ClientServerInterface* clientServerInterface)
    {
        auto* queueController = &clientServerInterface->queueController();

        connect(
            queueController, &QueueController::queueEntryAdded,
            this,
            [this](qint32 index, quint32 queueId, RequestID requestId)
            {
                Q_UNUSED(index)
                Q_UNUSED(queueId)

                if (requestId == _requestId)
                    setCommandExecutionSuccessful();
            }
        );
        connect(
            queueController, &QueueController::queueEntryInsertionFailed,
            this,
            [this](ResultMessageErrorCode errorCode, RequestID requestId)
            {
                if (requestId == _requestId)
                    setCommandExecutionResult(errorCode);
            }
        );
    }

    void QueueInsertSpecialItemCommand::start(
                                             ClientServerInterface* clientServerInterface)
    {
        auto& queueController = clientServerInterface->queueController();

        _requestId =
                queueController.insertSpecialItemAtIndex(_itemType, _index, _indexType);
    }

    /* ===== QueueDeleteCommand ===== */

    QueueDeleteCommand::QueueDeleteCommand(quint32 queueId)
     : _queueId(queueId),
       _wasDeleted(false)
    {
        //
    }

    bool QueueDeleteCommand::requiresAuthentication() const
    {
        return true;
    }

    void QueueDeleteCommand::setUp(ClientServerInterface* clientServerInterface)
    {
        auto* queueController = &clientServerInterface->queueController();

        connect(
            queueController, &QueueController::queueEntryRemoved,
            this,
            [this](qint32 index, quint32 queueId)
            {
                Q_UNUSED(index)

                if (queueId != _queueId)
                    return; /* not the one we deleted */

                _wasDeleted = true;
                listenerSlot();
            }
        );

        addStep(
            [this]() -> bool
            {
                if (_wasDeleted)
                    setCommandExecutionSuccessful();

                return false;
            }
        );
    }

    void QueueDeleteCommand::start(ClientServerInterface* clientServerInterface)
    {
        clientServerInterface->queueController().deleteQueueEntry(_queueId);
    }

    /* ===== QueueMoveCommand ===== */

    QueueMoveCommand::QueueMoveCommand(quint32 queueId, qint16 moveOffset)
     : _queueId(queueId),
       _moveOffset(moveOffset),
       _wasMoved(false)
    {
        //
    }

    bool QueueMoveCommand::requiresAuthentication() const
    {
        return true;
    }

    void QueueMoveCommand::setUp(ClientServerInterface* clientServerInterface)
    {
        auto* queueController = &clientServerInterface->queueController();

        connect(
            queueController, &QueueController::queueEntryMoved,
            this,
            [this](qint32 fromIndex, qint32 toIndex, quint32 queueId)
            {
                int movedOffset = toIndex - fromIndex;

                if (queueId != _queueId || movedOffset != _moveOffset)
                    return; /* not the move we asked for */

                _wasMoved = true;
                listenerSlot();
            }
        );

        addStep(
            [this]() -> bool
            {
                if (_wasMoved)
                    setCommandExecutionSuccessful();

                return false;
            }
        );
    }

    void QueueMoveCommand::start(ClientServerInterface* clientServerInterface)
    {
        clientServerInterface->queueController().moveQueueEntry(_queueId, _moveOffset);
    }

    /* ===== TrackStatsCommand ===== */

    TrackStatsCommand::TrackStatsCommand(const FileHash& hash)
     : _hash(hash)
    {
        //
    }

    bool TrackStatsCommand::requiresAuthentication() const
    {
        return true;
    }

    void TrackStatsCommand::setUp(ClientServerInterface* clientServerInterface)
    {
        auto userDataFetcher = &clientServerInterface->userDataFetcher();

        connect(userDataFetcher, &UserDataFetcher::dataReceivedForUser,
                this, &TrackStatsCommand::listenerSlot);

        auto userId = clientServerInterface->userLoggedInId();
        auto username = clientServerInterface->userLoggedInName();

        addStep(
            [this, userDataFetcher, userId, username]() -> bool
            {
                auto* hashData = userDataFetcher->getHashDataForUser(userId, _hash);
                if (hashData == nullptr)
                    return false;

                QString output;
                output.reserve(100);

                output += QString("Hash: %1\n").arg(_hash.toString());
                output += QString("User: %1\n").arg(username);

                QString lastHeard;
                if (!hashData->previouslyHeardReceived)
                    lastHeard = "unknown";
                else if (hashData->previouslyHeard.isNull())
                    lastHeard = "never";
                else
                    lastHeard =
                        hashData->previouslyHeard.toLocalTime().toString(Qt::RFC2822Date);

                output += QString("Last heard: %1\n").arg(lastHeard);

                QString score;
                if (!hashData->scoreReceived)
                    score = "unknown";
                else if (hashData->scorePermillage < 0)
                    score = "N/A";
                else
                    score = QString::number(hashData->scorePermillage / 10.0, 'f', 1);

                output += QString("Score: %1").arg(score);

                setCommandExecutionSuccessful(output);
                return false;
            }
        );
    }

    void TrackStatsCommand::start(ClientServerInterface* clientServerInterface)
    {
        Q_UNUSED(clientServerInterface)
        // no specific start action needed
    }
}
