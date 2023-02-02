/*
    Copyright (C) 2020-2022, Kevin Andre <hyperquantum@gmail.com>

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
#include "common/queueindextype.h"
#include "common/requestid.h"
#include "common/specialqueueitemtype.h"

#include "commandbase.h"

#include <QDateTime>

namespace PMP::Client
{
    class AbstractQueueMonitor;
    class QueueEntryInfo;
    class QueueEntryInfoStorage;
}

namespace PMP
{
    // TODO : remove useless constructors

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
        PlayCommand();

        bool requiresAuthentication() const override;

    protected:
        void run(Client::ServerInterface* serverInterface) override;
    };

    class PauseCommand : public CommandBase
    {
        Q_OBJECT
    public:
        PauseCommand();

        bool requiresAuthentication() const override;

    protected:
        void run(Client::ServerInterface* serverInterface) override;
    };

    class SkipCommand : public CommandBase
    {
        Q_OBJECT
    public:
        SkipCommand();

        bool requiresAuthentication() const override;

    protected:
        void run(Client::ServerInterface* serverInterface) override;

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
        void run(Client::ServerInterface* serverInterface) override;
    };

    class QueueCommand : public CommandBase
    {
        Q_OBJECT
    public:
        QueueCommand();

        bool requiresAuthentication() const override;

    protected:
        void run(Client::ServerInterface* serverInterface) override;

    private:
        void printQueue(Client::AbstractQueueMonitor* queueMonitor,
                        Client::QueueEntryInfoStorage* queueEntryInfoStorage);
        QString getSpecialEntryText(Client::QueueEntryInfo const* entry) const;

        int _fetchLimit;
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
        GetVolumeCommand();

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

    class BreakCommand : public CommandBase
    {
        Q_OBJECT
    public:
        BreakCommand();

        bool requiresAuthentication() const override;

    protected:
        void run(Client::ServerInterface* serverInterface) override;
    };

    class QueueInsertSpecialItemCommand : public CommandBase
    {
        Q_OBJECT
    public:
        QueueInsertSpecialItemCommand(SpecialQueueItemType itemType, int index,
                                      QueueIndexType indexType);

        bool requiresAuthentication() const override;

    protected:
        void run(Client::ServerInterface* serverInterface) override;

    private:
        SpecialQueueItemType _itemType;
        int _index;
        QueueIndexType _indexType;
        RequestID _requestId;
    };

    class QueueDeleteCommand : public CommandBase
    {
        Q_OBJECT
    public:
        QueueDeleteCommand(quint32 queueId);

        bool requiresAuthentication() const override;

    protected:
        void run(Client::ServerInterface* serverInterface) override;

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
        void run(Client::ServerInterface* serverInterface) override;

    private:
        quint32 _queueId;
        qint16 _moveOffset;
        bool _wasMoved;
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
