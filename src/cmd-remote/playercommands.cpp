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

#include "playercommands.h"

#include "common/util.h"

#include "client/currenttrackmonitor.h"
#include "client/localhashidrepository.h"
#include "client/playercontroller.h"
#include "client/serverinterface.h"

using namespace PMP::Client;

namespace PMP
{
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

    void DelayedStartAtCommand::run(ServerInterface* serverInterface)
    {
        auto future =
            serverInterface->playerController().activateDelayedStart(_startTime);

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

    void DelayedStartWaitCommand::run(ServerInterface* serverInterface)
    {
        auto future =
            serverInterface->playerController().activateDelayedStart(
                                                                      _delayMilliseconds);
        addCommandExecutionFutureListener(future);
    }

    /* ===== DeactivateDelayedCancelCommand ===== */

    bool DelayedStartCancelCommand::requiresAuthentication() const
    {
        return true;
    }

    void DelayedStartCancelCommand::run(ServerInterface* serverInterface)
    {
        auto future = serverInterface->playerController().deactivateDelayedStart();
        addCommandExecutionFutureListener(future);
    }

    /* ===== PlayCommand ===== */

    bool PlayCommand::requiresAuthentication() const
    {
        return true;
    }

    void PlayCommand::run(ServerInterface* serverInterface)
    {
        auto* playerController = &serverInterface->playerController();

        connect(playerController, &PlayerController::playerStateChanged,
                this, &PlayCommand::listenerSlot);

        addStep(
            [playerController]() -> StepResult
            {
                if (playerController->playerState() == PlayerState::Playing)
                    return StepResult::commandSuccessful();

                return StepResult::stepIncomplete();
            }
        );

        playerController->play();
    }

    /* ===== PauseCommand ===== */

    bool PauseCommand::requiresAuthentication() const
    {
        return true;
    }

    void PauseCommand::run(ServerInterface* serverInterface)
    {
        auto* playerController = &serverInterface->playerController();

        connect(playerController, &PlayerController::playerStateChanged,
                this, &PauseCommand::listenerSlot);

        addStep(
            [playerController]() -> StepResult
            {
                if (playerController->playerState() == PlayerState::Paused)
                    return StepResult::commandSuccessful();

                return StepResult::stepIncomplete();
            }
        );

        playerController->pause();
    }

    /* ===== SkipCommand ===== */

    bool SkipCommand::requiresAuthentication() const
    {
        return true;
    }

    void SkipCommand::run(ServerInterface* serverInterface)
    {
        auto* playerController = &serverInterface->playerController();

        connect(playerController, &PlayerController::playerStateChanged,
                this, &SkipCommand::listenerSlot);
        connect(playerController, &PlayerController::currentTrackChanged,
                this, &SkipCommand::listenerSlot);

        addStep(
            [this, playerController]() -> StepResult
            {
                if (playerController->playerState() == PlayerState::Unknown)
                    return StepResult::stepIncomplete();

                if (playerController->canSkip())
                {
                    _currentQueueId = playerController->currentQueueId();
                    playerController->skip();
                    return StepResult::stepCompleted();
                }
                else
                    return StepResult::commandFailed(3, "player cannot skip now");
            }
        );
        addStep(
            [this, playerController]() -> StepResult
            {
                if (playerController->currentQueueId() != _currentQueueId)
                    return StepResult::commandSuccessful();

                return StepResult::stepIncomplete();
            }
        );
    }

    /* ===== NowPlayingCommand ===== */

    bool NowPlayingCommand::requiresAuthentication() const
    {
        return false;
    }

    void NowPlayingCommand::run(ServerInterface* serverInterface)
    {
        auto* currentTrackMonitor = &serverInterface->currentTrackMonitor();
        auto* hashIdRepository = serverInterface->hashIdRepository();

        connect(currentTrackMonitor, &CurrentTrackMonitor::currentTrackChanged,
                this, &NowPlayingCommand::listenerSlot);
        connect(currentTrackMonitor, &CurrentTrackMonitor::currentTrackInfoChanged,
                this, &NowPlayingCommand::listenerSlot);

        addStep(
            [currentTrackMonitor, hashIdRepository]() -> StepResult
            {
                auto isTrackPresent = currentTrackMonitor->isTrackPresent();

                if (isTrackPresent.isFalse())
                    return StepResult::commandSuccessful("Now playing: nothing");

                if (isTrackPresent.isUnknown()
                        || currentTrackMonitor->currentTrackHash().isZero())
                {
                    return StepResult::stepIncomplete();
                }

                auto title = currentTrackMonitor->currentTrackTitle();
                auto artist = currentTrackMonitor->currentTrackArtist();
                //auto album = currentTrackMonitor->currentTrackAlbum(); NOT AVAILABLE YET
                auto possibleFileName =
                        currentTrackMonitor->currentTrackPossibleFilename();

                if (title.isEmpty() && artist.isEmpty() && possibleFileName.isEmpty())
                    return StepResult::stepIncomplete();

                auto queueId = currentTrackMonitor->currentQueueId();
                auto lengthMilliseconds =
                        currentTrackMonitor->currentTrackLengthMilliseconds();
                auto lengthString =
                        lengthMilliseconds < 0
                            ? ""
                            : Util::millisecondsToLongDisplayTimeText(lengthMilliseconds);
                auto hash =
                    hashIdRepository->getHash(currentTrackMonitor->currentTrackHash());

                QString output;
                output.reserve(100);
                output += "Now playing: track\n";
                output += " QID: " + QString::number(queueId) + "\n";
                output += " title: " + title + "\n";
                output += " artist: " + artist + "\n";
                //output += " album: " + album + "\n";
                output += " length: " + lengthString + "\n";

                if (title.isEmpty() && artist.isEmpty())
                    output += " possible filename: " + possibleFileName + "\n";

                output += " hash: " + hash.toString(); // no newline at the end here

                return StepResult::commandSuccessful(output);
            }
        );
    }

}
