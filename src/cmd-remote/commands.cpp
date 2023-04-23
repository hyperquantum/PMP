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

#include "commands.h"

#include "common/util.h"

#include "client/authenticationcontroller.h"
#include "client/currenttrackmonitor.h"
#include "client/dynamicmodecontroller.h"
#include "client/generalcontroller.h"
#include "client/localhashidrepository.h"
#include "client/playercontroller.h"
#include "client/serverinterface.h"
#include "client/userdatafetcher.h"

using namespace PMP::Client;

namespace PMP
{
    /* ===== StatusCommand ===== */

    bool StatusCommand::requiresAuthentication() const
    {
        return false;
    }

    void StatusCommand::run(Client::ServerInterface* serverInterface)
    {
        auto* playerController = &serverInterface->playerController();
        auto* currentTrackMonitor = &serverInterface->currentTrackMonitor();
        auto* dynamicModeController = &serverInterface->dynamicModeController();

        connect(playerController, &PlayerController::playerStateChanged,
                this, &StatusCommand::listenerSlot);
        connect(playerController, &PlayerController::playerModeChanged,
                this, &StatusCommand::listenerSlot);
        connect(currentTrackMonitor, &CurrentTrackMonitor::currentTrackChanged,
                this, &StatusCommand::listenerSlot);
        connect(dynamicModeController, &DynamicModeController::dynamicModeEnabledChanged,
                this, &StatusCommand::listenerSlot);

        addStep(
            [this, playerController, currentTrackMonitor, dynamicModeController]()
            {
                if (playerController->playerState() == PlayerState::Unknown
                        || playerController->playerMode() == PlayerMode::Unknown
                        || dynamicModeController->dynamicModeEnabled().isUnknown())
                {
                    return StepResult::stepIncomplete();
                }

                return printStatus(playerController, currentTrackMonitor,
                                   dynamicModeController);
            }
        );
    }

    CommandBase::StepResult StatusCommand::printStatus(
                                    Client::PlayerController* playerController,
                                    Client::CurrentTrackMonitor* currentTrackMonitor,
                                    Client::DynamicModeController* dynamicModeController)
    {
        QString output;
        output.reserve(40 * 9);

        auto trackLoaded = currentTrackMonitor->isTrackPresent();
        if (trackLoaded.isTrue())
            output += "track loaded: yes";
        else if (trackLoaded.isFalse())
            output += "track loaded: no";
        else
            output += "track loaded: (unknown)";

        output += "\n";
        switch (playerController->playerState())
        {
        case PlayerState::Playing:
            output += "playing: yes";
            output += "\n";
            output += "paused: no";
            break;
        case PlayerState::Stopped:
            output += "playing: no";
            output += "\n";
            output += "paused: no";
            break;
        case PlayerState::Paused:
            output += "playing: no";
            output += "\n";
            output += "paused: yes";
            break;
        case PlayerState::Unknown:
        default:
            qWarning() << "Player state is unknown or unhandled value";
            output += "playing: (unknown)";
            output += "\n";
            output += "paused: (unknown)";
            break;
        }

        output += "\n";
        auto volume = playerController->volume();
        if (volume >= 0)
            output += QString("volume: %1").arg(QString::number(volume));
        else
            output += "volume: (unknown)";

        output += "\n";
        output += "queue length: ";
        output += QString::number(playerController->queueLength());

        output += "\n";
        switch (playerController->playerMode())
        {
        case PlayerMode::Personal:
            output += "public mode: no";
            output += "\n";
            output += "personal mode: yes";
            output += "\n";
            output += "personal mode user: ";
            output += playerController->personalModeUserLogin();
            break;
        case PlayerMode::Public:
            output += "public mode: yes";
            output += "\n";
            output += "personal mode: no";
            output += "\n";
            output += "personal mode user: N/A";
            break;
        case PlayerMode::Unknown:
        default:
            output += "public mode: (unknown)";
            output += "\n";
            output += "personal mode: (unknown)";
            output += "\n";
            output += "personal mode user: (unknown)";
            break;
        }

        output += "\n";
        auto dynamicModeEnabled = dynamicModeController->dynamicModeEnabled();
        if (dynamicModeEnabled.isTrue())
            output += "dynamic mode: on";
        else if (dynamicModeEnabled.isFalse())
            output += "dynamic mode: off";
        else
            output += "dynamic mode: (unknown)";

        return StepResult::commandSuccessful(output);
    }

    /* ===== ServerVersionCommand ===== */

    bool ServerVersionCommand::requiresAuthentication() const
    {
        return false;
    }

    void ServerVersionCommand::run(ServerInterface* serverInterface)
    {
        auto future = serverInterface->generalController().getServerVersionInfo();

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

    void ReloadServerSettingsCommand::run(ServerInterface* serverInterface)
    {
        auto future = serverInterface->generalController().reloadServerSettings();
        addCommandExecutionFutureListener(future);
    }

    /* ===== PersonalModeCommand ===== */

    bool PersonalModeCommand::requiresAuthentication() const
    {
        return true;
    }

    void PersonalModeCommand::run(Client::ServerInterface* serverInterface)
    {
        auto myUserId = serverInterface->authenticationController().userLoggedInId();

        auto* playerController = &serverInterface->playerController();
        connect(playerController, &PlayerController::playerModeChanged,
                this, &PersonalModeCommand::listenerSlot);

        addStep(
            [myUserId, playerController]() -> StepResult
            {
                auto playerMode = playerController->playerMode();

                if (playerMode != PlayerMode::Personal
                        || playerController->personalModeUserId() != myUserId)
                {
                    return StepResult::stepIncomplete();
                }

                return StepResult::commandSuccessful();
            }
        );

        playerController->switchToPersonalMode();
    }

    /* ===== PublicModeCommand ===== */

    bool PublicModeCommand::requiresAuthentication() const
    {
        return true;
    }

    void PublicModeCommand::run(Client::ServerInterface* serverInterface)
    {
        auto* playerController = &serverInterface->playerController();
        connect(playerController, &PlayerController::playerModeChanged,
                this, &PublicModeCommand::listenerSlot);

        addStep(
            [playerController]() -> StepResult
            {
                auto playerMode = playerController->playerMode();

                if (playerMode == PlayerMode::Public)
                    return StepResult::commandSuccessful();

                return StepResult::stepIncomplete();
            }
        );

        playerController->switchToPublicMode();
    }

    /* ===== DynamicModeActivationCommand ===== */

    DynamicModeActivationCommand::DynamicModeActivationCommand(bool enable)
     : _enable(enable)
    {
        //
    }

    bool DynamicModeActivationCommand::requiresAuthentication() const
    {
        return true;
    }

    void DynamicModeActivationCommand::run(Client::ServerInterface* serverInterface)
    {
        auto* dynamicModeController = &serverInterface->dynamicModeController();

        connect(dynamicModeController, &DynamicModeController::dynamicModeEnabledChanged,
                this, &DynamicModeActivationCommand::listenerSlot);

        addStep(
            [this, dynamicModeController]() -> StepResult
            {
                if (dynamicModeController->dynamicModeEnabled().isIdenticalTo(_enable))
                    return StepResult::commandSuccessful();

                return StepResult::stepIncomplete();
            }
        );

        dynamicModeController->setDynamicModeEnabled(_enable);
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

    void ShutdownCommand::run(ServerInterface* serverInterface)
    {
        connect(serverInterface, &ServerInterface::connectedChanged,
                this, &ShutdownCommand::listenerSlot);

        addStep(
            [serverInterface]() -> StepResult
            {
                if (!serverInterface->connected())
                    return StepResult::commandSuccessful();

                return StepResult::stepIncomplete();
            }
        );

        serverInterface->generalController().shutdownServer();
    }

    /* ===== GetVolumeCommand ===== */

    bool GetVolumeCommand::requiresAuthentication() const
    {
        return false;
    }

    void GetVolumeCommand::run(ServerInterface* serverInterface)
    {
        auto* playerController = &serverInterface->playerController();

        connect(playerController, &PlayerController::volumeChanged,
                this, &GetVolumeCommand::listenerSlot);

        addStep(
            [playerController]() -> StepResult
            {
                auto volume = playerController->volume();

                if (volume >= 0)
                {
                    auto output = "Volume: " + QString::number(volume);
                    return StepResult::commandSuccessful(output);
                }

                return StepResult::stepIncomplete();
            }
        );
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

    void SetVolumeCommand::run(ServerInterface* serverInterface)
    {
        auto* playerController = &serverInterface->playerController();

        connect(playerController, &PlayerController::volumeChanged,
                this, &SetVolumeCommand::listenerSlot);

        addStep(
            [this, playerController]() -> StepResult
            {
                if (playerController->volume() == _volume)
                    return StepResult::commandSuccessful();

                return StepResult::stepIncomplete();
            }
        );

        playerController->setVolume(_volume);
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

    void TrackStatsCommand::run(ServerInterface* serverInterface)
    {
        auto hashId = serverInterface->hashIdRepository()->getOrRegisterId(_hash);

        auto userDataFetcher = &serverInterface->userDataFetcher();

        connect(userDataFetcher, &UserDataFetcher::dataReceivedForUser,
                this, &TrackStatsCommand::listenerSlot);

        auto userId = serverInterface->userLoggedInId();
        auto username = serverInterface->userLoggedInName();

        addStep(
            [this, userDataFetcher, userId, username, hashId]() -> StepResult
            {
                auto* hashData = userDataFetcher->getHashDataForUser(userId, hashId);
                if (hashData == nullptr)
                    return StepResult::stepIncomplete();

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

                return StepResult::commandSuccessful(output);
            }
        );
    }
}
