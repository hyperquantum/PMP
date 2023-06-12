/*
    Copyright (C) 2022-2023, Kevin Andre <hyperquantum@gmail.com>

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

#include "scrobblingcommands.h"

#include "client/scrobblingcontroller.h"
#include "client/serverinterface.h"

using namespace PMP::Client;

namespace PMP
{
    /* ===== ScrobblingActivationCommand ===== */

    ScrobblingActivationCommand::ScrobblingActivationCommand(ScrobblingProvider provider,
                                                             bool enable)
     : _provider(provider),
       _enable(enable)
    {
        //
    }

    bool ScrobblingActivationCommand::requiresAuthentication() const
    {
        return true;
    }

    void ScrobblingActivationCommand::run(Client::ServerInterface* serverInterface)
    {
        auto* scrobblingController = &serverInterface->scrobblingController();

        connect(scrobblingController, &ScrobblingController::lastFmInfoChanged,
                this, &ScrobblingActivationCommand::listenerSlot);

        addStep(
            [this, scrobblingController]() -> StepResult
            {
                if (scrobblingController->lastFmEnabled() == _enable)
                    return StepResult::commandSuccessful();

                return StepResult::stepIncomplete();
            }
        );

        serverInterface->scrobblingController().setLastFmScrobblingEnabled(_enable);
    }

    /* ===== ScrobblingStatusCommand ===== */

    ScrobblingStatusCommand::ScrobblingStatusCommand(ScrobblingProvider provider)
     : _provider(provider)
    {
        //
    }

    bool ScrobblingStatusCommand::requiresAuthentication() const
    {
        return true;
    }

    void ScrobblingStatusCommand::run(Client::ServerInterface* serverInterface)
    {
        auto* scrobblingController = &serverInterface->scrobblingController();

        connect(scrobblingController, &ScrobblingController::lastFmInfoChanged,
                this, &ScrobblingStatusCommand::listenerSlot);

        addStep(
            [scrobblingController]() -> StepResult
            {
                if (scrobblingController->lastFmEnabled() == null)
                    return StepResult::stepIncomplete();

                if (scrobblingController->lastFmEnabled() == false)
                    return StepResult::commandSuccessful("disabled");

                switch (scrobblingController->lastFmStatus())
                {
                case ScrobblerStatus::Unknown:
                    return StepResult::commandSuccessful("unknown"); /* FIXME */

                case ScrobblerStatus::Green:
                    return StepResult::commandSuccessful("green");

                case ScrobblerStatus::Yellow:
                    return StepResult::commandSuccessful("yellow");

                case ScrobblerStatus::Red:
                    return StepResult::commandSuccessful("red");

                case ScrobblerStatus::WaitingForUserCredentials:
                    return StepResult::commandSuccessful("waiting for user credentials");
                }

                return StepResult::stepIncomplete();
            }
        );
    }

    /* ===== ScrobblingAuthenticateCommand =====*/

    ScrobblingAuthenticateCommand::ScrobblingAuthenticateCommand(
                                                            ScrobblingProvider provider)
     : _provider(provider)
    {
        CredentialsPrompt prompt;
        prompt.providerName = toString(provider);

        enableInteractiveCredentialsPrompt(prompt);
    }

    bool ScrobblingAuthenticateCommand::requiresAuthentication() const
    {
        return true;
    }

    void ScrobblingAuthenticateCommand::run(Client::ServerInterface* serverInterface)
    {
        auto credentials = getCredentialsEntered();

        auto future =
            serverInterface->scrobblingController().authenticateLastFm(
                credentials.username, credentials.password
            );

        addCommandExecutionFutureListener(future);
    }
}
