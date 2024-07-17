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

#include "administrativecommands.h"

#include "client/generalcontroller.h"
#include "client/serverinterface.h"

using namespace PMP::Client;

namespace PMP
{
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

    /* ===== StartFullIndexationCommand ===== */

    void StartFullIndexationCommand::run(Client::ServerInterface* serverInterface)
    {
        auto future = serverInterface->generalController().startFullIndexation();
        setCommandExecutionResultFuture(future);
    }

    /* ===== StartQuickScanForNewFilesCommand ===== */

    void StartQuickScanForNewFilesCommand::run(Client::ServerInterface* serverInterface)
    {
        auto future = serverInterface->generalController().startQuickScanForNewFiles();
        setCommandExecutionResultFuture(future);
    }

    /* ===== ReloadServerSettingsCommand ===== */

    void ReloadServerSettingsCommand::run(ServerInterface* serverInterface)
    {
        auto future = serverInterface->generalController().reloadServerSettings();
        setCommandExecutionResultFuture(future);
    }

    /* ===== ShutdownCommand ===== */

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
}
