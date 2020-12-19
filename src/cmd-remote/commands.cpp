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

#include "commands.h"

#include "common/clientserverinterface.h"
#include "common/currenttrackmonitor.h"
#include "common/playercontroller.h"

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
                    setCommandExecutionFailed(3, "command timed out");
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
            executionFailed(3, "internal error");
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

    void NowPlayingCommand::execute(ClientServerInterface* clientServerInterface)
    {
        auto& currentTrackMonitor = clientServerInterface->currentTrackMonitor();

        if (currentTrackMonitor.currentQueueId() == 0)
        {
            Q_EMIT executionSuccessful("now playing: nothing");
            return;
        }

        // TODO
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

    /*
    GetVolumeCommand::GetVolumeCommand()
    {
        //
    }

    QString GetVolumeCommand::commandStringToSend() const
    {
        return "volume";
    }

    bool GetVolumeCommand::mustWaitForResponseAfterSending() const
    {
        return true;
    }
    */

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

    /* ===== QueueMoveCommand ===== */

    /*
    QueueMoveCommand::QueueMoveCommand(int queueId, int moveOffset)
     : _queueId(queueId),
       _moveOffset(moveOffset)
    {
        //
    }

    QString QueueMoveCommand::commandStringToSend() const
    {
        return "qmove " + QString::number(_queueId)
                + " "
                + (_moveOffset > 0 ? "+" : "")
                + QString::number(_moveOffset);
    }

    bool QueueMoveCommand::mustWaitForResponseAfterSending() const
    {
        return false;
    }
    */

}
