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

#ifndef PMP_ADMINISTRATIVECOMMANDS_H
#define PMP_ADMINISTRATIVECOMMANDS_H

#include "common/requestid.h"

#include "commandbase.h"

namespace PMP
{
    struct VersionInfo;

    class ServerVersionCommand : public CommandBase
    {
        Q_OBJECT
    public:
        bool requiresAuthentication() const override;

    protected:
        void run(Client::ServerInterface* serverInterface) override;

    private:
        void printVersion(VersionInfo const& versionInfo);
    };

    class StartFullIndexationCommand : public CommandBase
    {
        Q_OBJECT
    protected:
        void run(Client::ServerInterface* serverInterface) override;
    };

    class StartQuickScanForNewFilesCommand : public CommandBase
    {
        Q_OBJECT
    protected:
        void run(Client::ServerInterface* serverInterface) override;
    };

    class ReloadServerSettingsCommand : public CommandBase
    {
        Q_OBJECT
    protected:
        void run(Client::ServerInterface* serverInterface) override;

    private:
        RequestID _requestId;
    };

    class ShutdownCommand : public CommandBase
    {
        Q_OBJECT
    public:
        bool willCauseDisconnect() const override;

    protected:
        void run(Client::ServerInterface* serverInterface) override;
    };
}
#endif
