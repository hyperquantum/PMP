/*
    Copyright (C) 2022, Kevin Andre <hyperquantum@gmail.com>

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

    void ScrobblingActivationCommand::setUp(Client::ServerInterface* serverInterface)
    {
        auto* scrobblingController = &serverInterface->scrobblingController();

        connect(scrobblingController, &ScrobblingController::lastFmInfoChanged,
                this, &ScrobblingActivationCommand::listenerSlot);

        addStep(
            [this, scrobblingController]() -> bool
            {
                if (scrobblingController->lastFmEnabled() == _enable)
                    setCommandExecutionSuccessful();

                return false;
            }
        );
    }

    void ScrobblingActivationCommand::start(Client::ServerInterface* serverInterface)
    {
        serverInterface->scrobblingController().setLastFmScrobblingEnabled(_enable);
    }
}
