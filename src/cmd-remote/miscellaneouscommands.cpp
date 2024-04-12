/*
    Copyright (C) 2020-2024, Kevin Andre <hyperquantum@gmail.com>

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

#include "miscellaneouscommands.h"

#include "common/util.h"

#include "client/authenticationcontroller.h"
#include "client/collectionwatcher.h"
#include "client/currenttrackmonitor.h"
#include "client/dynamicmodecontroller.h"
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

    /* ===== PersonalModeCommand ===== */

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

    /* ===== TrackInfoCommand ===== */

    TrackInfoCommand::TrackInfoCommand(const FileHash& hash)
     : _hash(hash)
    {
        //
    }

    bool TrackInfoCommand::requiresAuthentication() const
    {
        return false;
    }

    void TrackInfoCommand::run(Client::ServerInterface* serverInterface)
    {
        auto collectionWatcher = &serverInterface->collectionWatcher();

        auto future = collectionWatcher->getTrackInfo(_hash);
        addFailureHandler(future);

        future.addResultListener(
            this,
            [this](CollectionTrackInfo trackInfo)
            {
                printTrackInfo(trackInfo);
            }
        );
    }

    void TrackInfoCommand::printTrackInfo(Client::CollectionTrackInfo& trackInfo)
    {
        QString output;
        output.reserve(100);

        output += QString("hash: %1\n").arg(_hash.toString());
        output += QString("title: %1\n").arg(trackInfo.title());
        output += QString("artist: %1\n").arg(trackInfo.artist());
        output += QString("album: %1\n").arg(trackInfo.album());
        output += QString("album artist: %1\n").arg(trackInfo.albumArtist());

        if (trackInfo.lengthIsKnown())
        {
            auto lengthText =
                Util::millisecondsToLongDisplayTimeText(trackInfo.lengthInMilliseconds());

            output += QString("length: %1").arg(lengthText);
        }
        else
        {
            output += "length: (unknown)";
        }

        output += "\n";

        if (trackInfo.isAvailable())
            output += "available: yes";
        else
            output += "available: no";

        setCommandExecutionSuccessful(output);
    }

    /* ===== TrackStatsCommand ===== */

    TrackStatsCommand::TrackStatsCommand(const FileHash& hash)
     : _hash(hash)
    {
        //
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
