/*
    Copyright (C) 2014-2022, Kevin Andre <hyperquantum@gmail.com>

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

#include "autopersonalmodeaction.h"

#include "client/playercontroller.h"
#include "client/serverinterface.h"

using namespace PMP::Client;

namespace PMP
{
    AutoPersonalModeAction::AutoPersonalModeAction(ServerInterface* serverInterface)
     : QObject(serverInterface),
       _serverInterface(serverInterface),
       _needToCheck(true)
    {
        auto playerController = &_serverInterface->playerController();

        connect(
            playerController, &PlayerController::playerStateChanged,
            this, [this]() { check(); }
        );
        connect(
            playerController, &PlayerController::playerModeChanged,
            this, [this]() { check(); }
        );

        check();
    }

    void AutoPersonalModeAction::check()
    {
        if (!_needToCheck)
            return;

        auto& playerController = _serverInterface->playerController();
        auto playerState = playerController.playerState();
        auto playerMode = playerController.playerMode();

        if (playerState == PlayerState::Unknown || playerMode == PlayerMode::Unknown)
            return;

        _needToCheck = false;

        if (playerState == PlayerState::Stopped && playerMode == PlayerMode::Public)
        {
            playerController.switchToPersonalMode();
        }
    }
}
