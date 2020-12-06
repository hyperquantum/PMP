/*
    Copyright (C) 2020, Kevin Andre <hyperquantum@gmail.com>

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

#include "command.h"

namespace PMP {

    class PlayCommand : public Command
    {
        Q_OBJECT
    public:
        PlayCommand();

        QString commandStringToSend() const override;
        bool mustWaitForResponseAfterSending() const override;
    };

    class PauseCommand : public Command
    {
        Q_OBJECT
    public:
        PauseCommand();

        QString commandStringToSend() const override;
        bool mustWaitForResponseAfterSending() const override;
    };

    class SkipCommand : public Command
    {
        Q_OBJECT
    public:
        SkipCommand();

        QString commandStringToSend() const override;
        bool mustWaitForResponseAfterSending() const override;
    };

    class NowPlayingCommand : public Command
    {
        Q_OBJECT
    public:
        NowPlayingCommand();

        QString commandStringToSend() const override;
        bool mustWaitForResponseAfterSending() const override;
    };

    class QueueCommand : public Command
    {
        Q_OBJECT
    public:
        QueueCommand();

        QString commandStringToSend() const override;
        bool mustWaitForResponseAfterSending() const override;
    };

    class ShutdownCommand : public Command
    {
        Q_OBJECT
    public:
        ShutdownCommand(QString serverPassword);

        QString commandStringToSend() const override;
        bool mustWaitForResponseAfterSending() const override;

    private:
        QString _serverPassword;
    };

    class GetVolumeCommand : public Command
    {
        Q_OBJECT
    public:
        GetVolumeCommand();

        QString commandStringToSend() const override;
        bool mustWaitForResponseAfterSending() const override;
    };

    class SetVolumeCommand : public Command
    {
        Q_OBJECT
    public:
        SetVolumeCommand(int volume);

        QString commandStringToSend() const override;
        bool mustWaitForResponseAfterSending() const override;

    private:
        int _volume;
    };

    class QueueMoveCommand : public Command
    {
        Q_OBJECT
    public:
        QueueMoveCommand(int queueId, int moveOffset);

        QString commandStringToSend() const override;
        bool mustWaitForResponseAfterSending() const override;

    private:
        int _queueId;
        int _moveOffset;
    };

}
#endif
