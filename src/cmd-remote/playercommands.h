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

#ifndef PMP_PLAYERCOMMANDS_H
#define PMP_PLAYERCOMMANDS_H

#include "common/requestid.h"

#include "commandbase.h"

namespace PMP
{
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
}
#endif
