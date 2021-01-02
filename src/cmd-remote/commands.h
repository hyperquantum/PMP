/*
    Copyright (C) 2020-2021, Kevin Andre <hyperquantum@gmail.com>

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

#include "common/playerstate.h"

#include "command.h"

#include <QVector>

#include <functional>

namespace PMP {

    class CommandBase : public Command
    {
        Q_OBJECT
    public:
        virtual void execute(ClientServerInterface* clientServerInterface) override;

    protected:
        CommandBase();

        void addStep(std::function<bool ()> step);
        void setCommandExecutionSuccessful(QString output = "");
        void setCommandExecutionFailed(int resultCode, QString errorOutput);

        virtual void setUp(ClientServerInterface* clientServerInterface) = 0;
        virtual void start(ClientServerInterface* clientServerInterface) = 0;

    protected Q_SLOTS:
        void listenerSlot();

    private:
        int _currentStep;
        QVector<std::function<bool ()>> _steps;
        bool _finishedOrFailed;
    };

    class PlayCommand : public CommandBase
    {
        Q_OBJECT
    public:
        PlayCommand();

        bool requiresAuthentication() const override;

    protected:
        void setUp(ClientServerInterface* clientServerInterface) override;
        void start(ClientServerInterface* clientServerInterface) override;
    };

    class PauseCommand : public CommandBase
    {
        Q_OBJECT
    public:
        PauseCommand();

        bool requiresAuthentication() const override;

    protected:
        void setUp(ClientServerInterface* clientServerInterface) override;
        void start(ClientServerInterface* clientServerInterface) override;
    };

    class SkipCommand : public CommandBase
    {
        Q_OBJECT
    public:
        SkipCommand();

        bool requiresAuthentication() const override;

    protected:
        void setUp(ClientServerInterface* clientServerInterface) override;
        void start(ClientServerInterface* clientServerInterface) override;

    private:
        quint32 _currentQueueId;
    };

    class NowPlayingCommand : public CommandBase
    {
        Q_OBJECT
    public:
        NowPlayingCommand();

        bool requiresAuthentication() const override;

    protected:
        void setUp(ClientServerInterface* clientServerInterface) override;
        void start(ClientServerInterface* clientServerInterface) override;
    };

    /*
    class QueueCommand : public Command
    {
        Q_OBJECT
    public:
        QueueCommand();

        void execute(ClientServerInterface* clientServerInterface) override;
    };
    */

    class ShutdownCommand : public Command
    {
        Q_OBJECT
    public:
        ShutdownCommand(/*QString serverPassword*/);

        bool requiresAuthentication() const override;
        void execute(ClientServerInterface* clientServerInterface) override;

    private:
        //QString _serverPassword;
    };

    class GetVolumeCommand : public CommandBase
    {
        Q_OBJECT
    public:
        GetVolumeCommand();

        bool requiresAuthentication() const override;

    protected:
        void setUp(ClientServerInterface* clientServerInterface) override;
        void start(ClientServerInterface* clientServerInterface) override;
    };

    class SetVolumeCommand : public CommandBase
    {
        Q_OBJECT
    public:
        SetVolumeCommand(int volume);

        bool requiresAuthentication() const override;

    protected:
        void setUp(ClientServerInterface* clientServerInterface) override;
        void start(ClientServerInterface* clientServerInterface) override;

    private:
        int _volume;
    };

    class QueueDeleteCommand : public CommandBase
    {
        Q_OBJECT
    public:
        QueueDeleteCommand(quint32 queueId);

        bool requiresAuthentication() const override;

    protected:
        void setUp(ClientServerInterface* clientServerInterface) override;
        void start(ClientServerInterface* clientServerInterface) override;

    private:
        quint32 _queueId;
        bool _wasDeleted;
    };

    class QueueMoveCommand : public CommandBase
    {
        Q_OBJECT
    public:
        QueueMoveCommand(quint32 queueId, qint16 moveOffset);

        bool requiresAuthentication() const override;

    protected:
        void setUp(ClientServerInterface* clientServerInterface) override;
        void start(ClientServerInterface* clientServerInterface) override;

    private:
        quint32 _queueId;
        qint16 _moveOffset;
        bool _wasMoved;
    };

}
#endif
