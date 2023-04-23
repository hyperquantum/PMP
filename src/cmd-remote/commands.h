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

#ifndef PMP_COMMANDS_H
#define PMP_COMMANDS_H

#include "common/filehash.h"
#include "common/requestid.h"

#include "commandbase.h"

#include <QDateTime>

namespace PMP::Client
{
    class AbstractQueueMonitor;
    class CurrentTrackMonitor;
    class DynamicModeController;
    class PlayerController;
    class QueueEntryInfo;
    class QueueEntryInfoStorage;
}

namespace PMP
{
    struct VersionInfo;

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

    class ReloadServerSettingsCommand : public CommandBase
    {
        Q_OBJECT
    public:
        ReloadServerSettingsCommand();

        bool requiresAuthentication() const override;

    protected:
        void run(Client::ServerInterface* serverInterface) override;

    private:
        RequestID _requestId;
    };

    class PersonalModeCommand : public CommandBase
    {
        Q_OBJECT
    public:
        bool requiresAuthentication() const override;

    protected:
        void run(Client::ServerInterface* serverInterface) override;
    };

    class PublicModeCommand : public CommandBase
    {
        Q_OBJECT
    public:
        bool requiresAuthentication() const override;

    protected:
        void run(Client::ServerInterface* serverInterface) override;
    };

    class DynamicModeActivationCommand : public CommandBase
    {
        Q_OBJECT
    public:
        explicit DynamicModeActivationCommand(bool enable);

        bool requiresAuthentication() const override;

    protected:
        void run(Client::ServerInterface* serverInterface) override;

    private:
        bool _enable;
    };

    class DelayedStartAtCommand : public CommandBase
    {
        Q_OBJECT
    public:
        DelayedStartAtCommand(QDateTime startTime);

        bool requiresAuthentication() const override;

    protected:
        void run(Client::ServerInterface* serverInterface) override;

    private:
        QDateTime _startTime;
        RequestID _requestId;
    };

    class DelayedStartWaitCommand : public CommandBase
    {
        Q_OBJECT
    public:
        DelayedStartWaitCommand(qint64 delayMilliseconds);

        bool requiresAuthentication() const override;

    protected:
        void run(Client::ServerInterface* serverInterface) override;

    private:
        qint64 _delayMilliseconds;
        RequestID _requestId;
    };

    class DelayedStartCancelCommand : public CommandBase
    {
        Q_OBJECT
    public:
        bool requiresAuthentication() const override;

    protected:
        void run(Client::ServerInterface* serverInterface) override;

    private:
        RequestID _requestId;
    };

    class PlayCommand : public CommandBase
    {
        Q_OBJECT
    public:
        bool requiresAuthentication() const override;

    protected:
        void run(Client::ServerInterface* serverInterface) override;
    };

    class PauseCommand : public CommandBase
    {
        Q_OBJECT
    public:
        bool requiresAuthentication() const override;

    protected:
        void run(Client::ServerInterface* serverInterface) override;
    };

    class SkipCommand : public CommandBase
    {
        Q_OBJECT
    public:
        bool requiresAuthentication() const override;

    protected:
        void run(Client::ServerInterface* serverInterface) override;

    private:
        quint32 _currentQueueId { 0 };
    };

    class NowPlayingCommand : public CommandBase
    {
        Q_OBJECT
    public:
        bool requiresAuthentication() const override;

    protected:
        void run(Client::ServerInterface* serverInterface) override;
    };

    class ShutdownCommand : public CommandBase
    {
        Q_OBJECT
    public:
        ShutdownCommand(/*QString serverPassword*/);

        bool requiresAuthentication() const override;
        bool willCauseDisconnect() const override;

    protected:
        void run(Client::ServerInterface* serverInterface) override;

    private:
        //QString _serverPassword;
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
        SetVolumeCommand(int volume);

        bool requiresAuthentication() const override;

    protected:
        void run(Client::ServerInterface* serverInterface) override;

    private:
        int _volume;
    };

    class TrackStatsCommand : public CommandBase
    {
        Q_OBJECT
    public:
        TrackStatsCommand(FileHash const& hash);

        bool requiresAuthentication() const override;

    protected:
        void run(Client::ServerInterface* serverInterface) override;

    private:
        FileHash _hash;
    };
}
#endif
