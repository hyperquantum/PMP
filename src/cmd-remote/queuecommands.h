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

#ifndef PMP_QUEUECOMMANDS_H
#define PMP_QUEUECOMMANDS_H

#include "common/filehash.h"
#include "common/queueindextype.h"
#include "common/requestid.h"
#include "common/specialqueueitemtype.h"

#include "commandbase.h"

namespace PMP::Client
{
    class AbstractQueueMonitor;
    class QueueEntryInfo;
    class QueueEntryInfoStorage;
}

namespace PMP
{
    class QueueCommand : public CommandBase
    {
        Q_OBJECT
    public:
        bool requiresAuthentication() const override;

    protected:
        void run(Client::ServerInterface* serverInterface) override;

    private:
        StepResult printQueue(Client::AbstractQueueMonitor* queueMonitor,
                              Client::QueueEntryInfoStorage* queueEntryInfoStorage);
        QString getSpecialEntryText(Client::QueueEntryInfo const* entry) const;

        int _fetchLimit { 10 };
    };

    class BreakCommand : public CommandBase
    {
        Q_OBJECT
    public:
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

    class QueueInsertTrackCommand : public CommandBase
    {
        Q_OBJECT
    public:
        QueueInsertTrackCommand(FileHash const& hash, int index,
                                QueueIndexType indexType);

        bool requiresAuthentication() const override;

    protected:
        void run(Client::ServerInterface* serverInterface) override;

    private:
        void insertNormal(Client::ServerInterface* serverInterface);
        void insertReversed(Client::ServerInterface* serverInterface);

        FileHash _hash;
        int _index;
        QueueIndexType _indexType;
        RequestID _requestId;
    };

    class InsertCommandBuilder
    {
    public:
        void setItem(SpecialQueueItemType specialItemType);
        void setItem(FileHash hash);

        void setPosition(QueueIndexType indexType, int index);

        Command* buildCommand();

    private:
        Nullable<SpecialQueueItemType> _queueItemType;
        QueueIndexType _indexType { QueueIndexType::Normal };
        int _index { -1 };
        FileHash _hash;
    };

    class QueueDeleteCommand : public CommandBase
    {
        Q_OBJECT
    public:
        explicit QueueDeleteCommand(quint32 queueId);

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
}
#endif
