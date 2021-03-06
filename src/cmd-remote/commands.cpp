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

#include "commands.h"

#include "common/clientserverinterface.h"
#include "common/currenttrackmonitor.h"
#include "common/playercontroller.h"
#include "common/queuecontroller.h"
#include "common/queueentryinfofetcher.h"
#include "common/queuemonitor.h"
#include "common/util.h"

#include <QtDebug>
#include <QTimer>

namespace PMP {

    /* ===== CommandBase ===== */

    void CommandBase::execute(ClientServerInterface* clientServerInterface)
    {
        setUp(clientServerInterface);

        start(clientServerInterface);
        qDebug() << "CommandBase: called start()";

        // initial quick check
        QTimer::singleShot(0, this, &CommandBase::listenerSlot);

        // set up timeout timer
        QTimer::singleShot(
            1000, this,
            [this]()
            {
                if (!_finishedOrFailed)
                {
                    qWarning() << "CommandBase: timeout triggered";
                    setCommandExecutionFailed(3, "Command timed out");
                }
            }
        );
    }

    CommandBase::CommandBase()
     : _currentStep(0),
       _stepDelayMilliseconds(0),
       _finishedOrFailed(false)
    {
        //
    }

    void CommandBase::addStep(std::function<bool ()> step)
    {
        _steps.append(step);
    }

    void CommandBase::setStepDelay(int milliseconds)
    {
        _stepDelayMilliseconds = milliseconds;
    }

    void CommandBase::setCommandExecutionSuccessful(QString output)
    {
        qDebug() << "CommandBase: command reported success";
        _finishedOrFailed = true;
        Q_EMIT executionSuccessful(output);
    }

    void CommandBase::setCommandExecutionFailed(int resultCode, QString errorOutput)
    {
        qDebug() << "CommandBase: command reported failure, code:" << resultCode;
        _finishedOrFailed = true;
        Q_EMIT executionFailed(resultCode, errorOutput);
    }

    void CommandBase::listenerSlot()
    {
        if (_finishedOrFailed)
            return;

        bool canAdvance = _steps[_currentStep]();

        if (_finishedOrFailed)
            return;

        if (!canAdvance)
            return;

        _currentStep++;

        if (_currentStep >= _steps.size())
        {
            qWarning() << "Step number" << _currentStep
                       << "should be less than" << _steps.size();
            Q_EMIT executionFailed(3, "internal error");
            return;
        }

        // we advanced, so try the next step right now, don't wait for signals
        QTimer::singleShot(_stepDelayMilliseconds, this, &CommandBase::listenerSlot);
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

        auto* queueEntryInfoFetcher = &clientServerInterface->queueEntryInfoFetcher();

        connect(queueMonitor, &QueueMonitor::fetchCompleted,
                this, &QueueCommand::listenerSlot);
        connect(queueEntryInfoFetcher, &QueueEntryInfoFetcher::tracksChanged,
                this, &QueueCommand::listenerSlot);

        addStep(
            [this, queueMonitor, queueEntryInfoFetcher]() -> bool
            {
                if (!queueMonitor->isFetchCompleted())
                    return false;

                bool needToWaitForFilename = false;
                for (int i = 0; i < queueMonitor->queueLength() && i < _fetchLimit; ++i)
                {
                    auto queueId = queueMonitor->queueEntry(i);
                    if (queueId == 0)
                        return false; /* download incomplete, shouldn't happen */

                    auto entry = queueEntryInfoFetcher->entryInfoByQID(queueId);
                    if (!entry)
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
            [this, queueMonitor, queueEntryInfoFetcher]() -> bool
            {
                printQueue(queueMonitor, queueEntryInfoFetcher);
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
                                  QueueEntryInfoFetcher* queueEntryInfoFetcher)
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

            auto entry = queueEntryInfoFetcher->entryInfoByQID(queueId);
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
            }
            else if (entry->isTrack().toBool(true))
            {
                output += "   ??   ";
            }
            else
            {
                output += "        ";
            }

            output += "|";

            if (entry->isTrack().toBool(false) == false)
            {
                output += "    ";
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
                return "---------- BREAK ----------";

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

    void ShutdownCommand::execute(ClientServerInterface* clientServerInterface)
    {
        clientServerInterface->shutdownServer();
        Q_EMIT executionSuccessful();
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

        auto* queueEntryInfoFetcher = &clientServerInterface->queueEntryInfoFetcher();

        connect(queueMonitor, &QueueMonitor::fetchCompleted,
                this, &BreakCommand::listenerSlot);
        connect(queueEntryInfoFetcher, &QueueEntryInfoFetcher::tracksChanged,
                this, &BreakCommand::listenerSlot);

        addStep(
            [this, queueMonitor, queueEntryInfoFetcher]() -> bool
            {
                if (!queueMonitor->isFetchCompleted())
                    return false;

                if (queueMonitor->queueLength() == 0)
                    return false;

                auto firstEntryId = queueMonitor->queueEntry(0);
                if (firstEntryId == 0)
                    return false; /* shouldn't happen */

                auto firstEntry = queueEntryInfoFetcher->entryInfoByQID(firstEntryId);
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
        clientServerInterface->queueController().insertBreakAtFront();
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

}
