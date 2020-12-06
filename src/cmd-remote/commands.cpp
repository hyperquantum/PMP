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

#include "commands.h"

namespace PMP {

    /* ===== PlayCommand ===== */

    PlayCommand::PlayCommand()
    {
        //
    }

    QString PlayCommand::commandStringToSend() const
    {
        return "play";
    }

    bool PlayCommand::mustWaitForResponseAfterSending() const
    {
        return false;
    }

    /* ===== PauseCommand ===== */

    PauseCommand::PauseCommand()
    {
        //
    }

    QString PauseCommand::commandStringToSend() const
    {
        return "pause";
    }

    bool PauseCommand::mustWaitForResponseAfterSending() const
    {
        return false;
    }

    /* ===== SkipCommand ===== */

    SkipCommand::SkipCommand()
    {
        //
    }

    QString SkipCommand::commandStringToSend() const
    {
        return "skip";
    }

    bool SkipCommand::mustWaitForResponseAfterSending() const
    {
        return false;
    }

    /* ===== NowPlayingCommand ===== */

    NowPlayingCommand::NowPlayingCommand()
    {
        //
    }

    QString NowPlayingCommand::commandStringToSend() const
    {
        return "nowplaying";
    }

    bool NowPlayingCommand::mustWaitForResponseAfterSending() const
    {
        return true;
    }

    /* ===== QueueCommand ===== */

    QueueCommand::QueueCommand()
    {
        //
    }

    QString QueueCommand::commandStringToSend() const
    {
        return "queue";
    }

    bool QueueCommand::mustWaitForResponseAfterSending() const
    {
        return true;
    }

    /* ===== ShutdownCommand ===== */

    ShutdownCommand::ShutdownCommand(QString serverPassword)
     : _serverPassword(serverPassword)
    {
        //
    }

    QString ShutdownCommand::commandStringToSend() const
    {
        return "shutdown " + _serverPassword;
    }

    bool ShutdownCommand::mustWaitForResponseAfterSending() const
    {
        return false;
    }

    /* ===== GetVolumeCommand ===== */

    GetVolumeCommand::GetVolumeCommand()
    {
        //
    }

    QString GetVolumeCommand::commandStringToSend() const
    {
        return "volume";
    }

    bool GetVolumeCommand::mustWaitForResponseAfterSending() const
    {
        return true;
    }

    /* ===== SetVolumeCommand ===== */

    SetVolumeCommand::SetVolumeCommand(int volume)
     : _volume(volume)
    {
        //
    }

    QString SetVolumeCommand::commandStringToSend() const
    {
        return "volume " + QString::number(_volume);
    }

    bool SetVolumeCommand::mustWaitForResponseAfterSending() const
    {
        return false;
    }

    /* ===== QueueMoveCommand ===== */

    QueueMoveCommand::QueueMoveCommand(int queueId, int moveOffset)
     : _queueId(queueId),
       _moveOffset(moveOffset)
    {
        //
    }

    QString QueueMoveCommand::commandStringToSend() const
    {
        return "qmove " + QString::number(_queueId)
                + " "
                + (_moveOffset > 0 ? "+" : "")
                + QString::number(_moveOffset);
    }

    bool QueueMoveCommand::mustWaitForResponseAfterSending() const
    {
        return false;
    }

}
