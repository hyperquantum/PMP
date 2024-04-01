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

#ifndef PMP_MISCELLANEOUSCOMMANDS_H
#define PMP_MISCELLANEOUSCOMMANDS_H

#include "common/filehash.h"

#include "commandbase.h"

#include <QDateTime>

namespace PMP::Client
{
    class CollectionTrackInfo;
    class CurrentTrackMonitor;
    class DynamicModeController;
    class PlayerController;
}

namespace PMP
{
    class StatusCommand : public CommandBase
    {
        Q_OBJECT
    public:
        bool requiresAuthentication() const override;

    protected:
        void run(Client::ServerInterface* serverInterface) override;

    private:
        StepResult printStatus(Client::PlayerController* playerController,
                               Client::CurrentTrackMonitor* currentTrackMonitor,
                               Client::DynamicModeController* dynamicModeController);
    };

    class PersonalModeCommand : public CommandBase
    {
        Q_OBJECT
    protected:
        void run(Client::ServerInterface* serverInterface) override;
    };

    class PublicModeCommand : public CommandBase
    {
        Q_OBJECT
    protected:
        void run(Client::ServerInterface* serverInterface) override;
    };

    class DynamicModeActivationCommand : public CommandBase
    {
        Q_OBJECT
    public:
        explicit DynamicModeActivationCommand(bool enable);

    protected:
        void run(Client::ServerInterface* serverInterface) override;

    private:
        bool _enable;
    };

    class GetVolumeCommand : public CommandBase
    {
        Q_OBJECT
    public:
        bool requiresAuthentication() const override;

    protected:
        void run(Client::ServerInterface* serverInterface) override;
    };

    class SetVolumeCommand : public CommandBase
    {
        Q_OBJECT
    public:
        explicit SetVolumeCommand(int volume);

    protected:
        void run(Client::ServerInterface* serverInterface) override;

    private:
        int _volume;
    };

    class TrackInfoCommand : public CommandBase
    {
        Q_OBJECT
    public:
        explicit TrackInfoCommand(FileHash const& hash);

        bool requiresAuthentication() const override;

    protected:
        void run(Client::ServerInterface* serverInterface) override;

    private:
        void printTrackInfo(Client::CollectionTrackInfo& trackInfo);

        FileHash _hash;
    };

    class TrackStatsCommand : public CommandBase
    {
        Q_OBJECT
    public:
        explicit TrackStatsCommand(FileHash const& hash);

    protected:
        void run(Client::ServerInterface* serverInterface) override;

    private:
        FileHash _hash;
    };
}
#endif
