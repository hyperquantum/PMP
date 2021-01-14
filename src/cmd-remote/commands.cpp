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
       _finishedOrFailed(false)
    {
        //
    }

    void CommandBase::addStep(std::function<bool ()> step)
    {
        _steps.append(step);
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
        QTimer::singleShot(0, this, &CommandBase::listenerSlot);
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

    /*
    QueueCommand::QueueCommand()
    {
        //
    }

    QString QueueCommand::commandStringToSend() const
    {
        return "queue";
    }

    bool QueueCommand::mustWaitForResponseAfterSending() const
    {
        return true;
    }
    */

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
