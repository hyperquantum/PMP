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

#include "queuecommands.h"

#include "common/util.h"

#include "client/localhashidrepository.h"
#include "client/queuecontroller.h"
#include "client/queueentryinfostorage.h"
#include "client/queuemonitor.h"
#include "client/serverinterface.h"

using namespace PMP::Client;

namespace PMP
{
    /* ===== QueueCommand ===== */

    bool QueueCommand::requiresAuthentication() const
    {
        return true;
    }

    void QueueCommand::run(ServerInterface* serverInterface)
    {
        auto* queueMonitor = &serverInterface->queueMonitor();
        queueMonitor->setFetchLimit(_fetchLimit);

        auto* queueEntryInfoStorage = &serverInterface->queueEntryInfoStorage();

        (void)serverInterface->queueEntryInfoFetcher(); /* speed things up */

        connect(queueMonitor, &QueueMonitor::fetchCompleted,
                this, &QueueCommand::listenerSlot);
        connect(queueEntryInfoStorage, &QueueEntryInfoStorage::tracksChanged,
                this, &QueueCommand::listenerSlot);

        addStep(
            [this, queueMonitor, queueEntryInfoStorage]() -> StepResult
            {
                if (!queueMonitor->isFetchCompleted())
                    return StepResult::stepIncomplete();

                bool needToWaitForFilename = false;
                for (int i = 0; i < queueMonitor->queueLength() && i < _fetchLimit; ++i)
                {
                    auto queueId = queueMonitor->queueEntry(i);
                    if (queueId == 0) /* download incomplete, shouldn't happen */
                        return StepResult::stepIncomplete();

                    auto entry = queueEntryInfoStorage->entryInfoByQueueId(queueId);
                    if (!entry || entry->type() == QueueEntryType::Unknown)
                        return StepResult::stepIncomplete(); /* info not available yet */

                    if (entry->needFilename())
                        needToWaitForFilename = true;
                }

                if (needToWaitForFilename)
                    setStepDelay(50);

                return StepResult::stepCompleted();
            }
        );
        addStep(
            [this, queueMonitor, queueEntryInfoStorage]() -> StepResult
            {
                return printQueue(queueMonitor, queueEntryInfoStorage);
            }
        );
    }

    CommandBase::StepResult QueueCommand::printQueue(AbstractQueueMonitor* queueMonitor,
                                             QueueEntryInfoStorage* queueEntryInfoStorage)
    {
        QString output;
        output.reserve(80 + 80 + 80 * _fetchLimit);

        output += "queue length " + QString::number(queueMonitor->queueLength()) + "\n";
        output += "Index|  QID  | Length | Title                          | Artist";

        for (int index = 0;
             index < queueMonitor->queueLength() && index < _fetchLimit;
             ++index)
        {
            output += "\n";
            output += QString::number(index).rightJustified(5);
            output += "|";

            auto queueId = queueMonitor->queueEntry(index);
            if (queueId == 0)
            {
                output += "??????????"; /* shouldn't happen */
                continue;
            }

            output += QString::number(queueId).rightJustified(7);
            output += "|";

            auto entry = queueEntryInfoStorage->entryInfoByQueueId(queueId);
            if (!entry)
            {
                output += "??????????"; /* info not available yet, unlikely but possible*/
                continue;
            }

            auto lengthMilliseconds = entry->lengthInMilliseconds();
            if (lengthMilliseconds >= 0)
            {
                auto lengthString =
                        Util::millisecondsToShortDisplayTimeText(lengthMilliseconds);
                output += lengthString.rightJustified(8);
                output += "|";
            }
            else if (entry->isTrack().toBool(true))
            {
                output += "   ??   ";
                output += "|";
            }
            else
            {
                output += "        ";
            }

            if (entry->isTrack().toBool(false) == false)
            {
                output += "      ";
                output += getSpecialEntryText(entry);
            }
            else if (entry->needFilename() && !entry->informativeFilename().isEmpty())
            {
                output += entry->informativeFilename();
            }
            else
            {
                output += entry->title().leftJustified(32);
                output += "|";
                output += entry->artist();
            }
        }

        if (_fetchLimit < queueMonitor->queueLength())
        {
            output += "\n...";
        }

        return StepResult::commandSuccessful(output);
    }

    QString QueueCommand::getSpecialEntryText(const QueueEntryInfo* entry) const
    {
        switch (entry->type())
        {
            case QueueEntryType::Track:
                /* shouldn't happen */
                return "";

            case QueueEntryType::BreakPoint:
                return "----------- BREAK -----------";

            case QueueEntryType::Barrier:
                return "---------- BARRIER ----------";

            case QueueEntryType::UnknownSpecialType:
                return "<<<< UNKNOWN ENTITY >>>>";

            case QueueEntryType::Unknown:
                return "???????????";
        }

        /* shouldn't happen */
        return "";
    }

    /* ===== BreakCommand =====*/

    bool BreakCommand::requiresAuthentication() const
    {
        return true;
    }

    void BreakCommand::run(ServerInterface* serverInterface)
    {
        auto* queueMonitor = &serverInterface->queueMonitor();
        queueMonitor->setFetchLimit(1);

        auto* queueEntryInfoStorage = &serverInterface->queueEntryInfoStorage();

        connect(queueMonitor, &QueueMonitor::fetchCompleted,
                this, &BreakCommand::listenerSlot);
        connect(queueEntryInfoStorage, &QueueEntryInfoStorage::tracksChanged,
                this, &BreakCommand::listenerSlot);

        addStep(
            [queueMonitor, queueEntryInfoStorage]() -> StepResult
            {
                if (!queueMonitor->isFetchCompleted())
                    return StepResult::stepIncomplete();

                if (queueMonitor->queueLength() == 0)
                    return StepResult::stepIncomplete();

                auto firstEntryId = queueMonitor->queueEntry(0);
                if (firstEntryId == 0)
                    return StepResult::stepIncomplete(); /* shouldn't happen */

                auto firstEntry = queueEntryInfoStorage->entryInfoByQueueId(firstEntryId);
                if (!firstEntry)
                    return StepResult::stepIncomplete();

                if (firstEntry->type() != QueueEntryType::BreakPoint)
                    return StepResult::stepIncomplete();

                return StepResult::commandSuccessful();
            }
        );

        serverInterface->queueController().insertBreakAtFrontIfNotExists();
    }

    /* ===== QueueInsertSpecialItemCommand ===== */

    QueueInsertSpecialItemCommand::QueueInsertSpecialItemCommand(
                                                            SpecialQueueItemType itemType,
                                                            int index,
                                                            QueueIndexType indexType)
     : _itemType(itemType),
       _index(index),
       _indexType(indexType)
    {
        //
    }

    bool QueueInsertSpecialItemCommand::requiresAuthentication() const
    {
        return true;
    }

    void QueueInsertSpecialItemCommand::run(ServerInterface* serverInterface)
    {
        auto* queueController = &serverInterface->queueController();

        connect(
            queueController, &QueueController::queueEntryAdded,
            this,
            [this](qint32 index, quint32 queueId, RequestID requestId)
            {
                Q_UNUSED(index)
                Q_UNUSED(queueId)

                if (requestId == _requestId)
                    setCommandExecutionSuccessful();
            }
        );
        connect(
            queueController, &QueueController::queueEntryInsertionFailed,
            this,
            [this](ResultMessageErrorCode errorCode, RequestID requestId)
            {
                if (requestId == _requestId)
                    setCommandExecutionResult(errorCode);
            }
        );

        _requestId =
                queueController->insertSpecialItemAtIndex(_itemType, _index, _indexType);
    }

    /* ===== QueueInsertTrackCommand ===== */

    QueueInsertTrackCommand::QueueInsertTrackCommand(const FileHash& hash, int index,
                                                     QueueIndexType indexType)
     : _hash(hash),
       _index(index),
       _indexType(indexType)
    {
        //
    }

    bool QueueInsertTrackCommand::requiresAuthentication() const
    {
        return true;
    }

    void QueueInsertTrackCommand::run(Client::ServerInterface* serverInterface)
    {
        auto* queueController = &serverInterface->queueController();

        connect(
            queueController, &QueueController::queueEntryAdded,
            this,
            [this](qint32 index, quint32 queueId, RequestID requestId)
            {
                Q_UNUSED(index)
                Q_UNUSED(queueId)

                if (requestId == _requestId)
                    setCommandExecutionSuccessful();
            }
        );
        connect(
            queueController, &QueueController::queueEntryInsertionFailed,
            this,
            [this](ResultMessageErrorCode errorCode, RequestID requestId)
            {
                if (requestId == _requestId)
                    setCommandExecutionResult(errorCode);
            }
        );

        if (_indexType == QueueIndexType::Reverse)
        {
            insertReversed(serverInterface);
        }
        else
        {
            insertNormal(serverInterface);
        }
    }

    void QueueInsertTrackCommand::insertNormal(Client::ServerInterface* serverInterface)
    {
        auto hashId = serverInterface->hashIdRepository()->getOrRegisterId(_hash);

        _requestId =
            serverInterface->queueController().insertQueueEntryAtIndex(hashId, _index);
    }

    void QueueInsertTrackCommand::insertReversed(Client::ServerInterface* serverInterface)
    {
        auto hashId = serverInterface->hashIdRepository()->getOrRegisterId(_hash);

        auto* queueController = &serverInterface->queueController();

        auto* queueMonitor = &serverInterface->queueMonitor();
        queueMonitor->setFetchLimit(1);

        connect(queueMonitor, &QueueMonitor::queueLengthChanged,
                this, &QueueInsertTrackCommand::listenerSlot);

        addStep(
            [this, hashId, queueMonitor, queueController]() -> StepResult
            {
                if (!queueMonitor->isQueueLengthKnown())
                    return StepResult::stepIncomplete();

                auto insertionIndex = queueMonitor->queueLength() - _index;
                if (insertionIndex < 0)
                {
                    auto error = ResultMessageErrorCode::InvalidQueueIndex;
                    return StepResult::commandFailed(error);
                }

                _requestId =
                    queueController->insertQueueEntryAtIndex(hashId, insertionIndex);

                return StepResult::stepCompleted();
            }
        );
    }

    /* ===== InsertCommandBuilder ===== */

    void InsertCommandBuilder::setItem(SpecialQueueItemType specialItemType)
    {
        _queueItemType = specialItemType;
        _hash = {};
    }

    void InsertCommandBuilder::setItem(FileHash hash)
    {
        Q_ASSERT_X(!hash.isNull(), "InsertCommandBuilder", "hash is null");

        _hash = hash;
    }

    void InsertCommandBuilder::setPosition(QueueIndexType indexType, int index)
    {
        Q_ASSERT_X(index >= 0, "InsertCommandBuilder", "index is negative");

        _indexType = indexType;
        _index = index;
    }

    Command* InsertCommandBuilder::buildCommand()
    {
        Q_ASSERT_X(_queueItemType.hasValue() || !_hash.isNull(),
                   "InsertCommandBuilder", "item is not set");

        Q_ASSERT_X(_index >= 0, "InsertCommandBuilder", "position is not set");

        if (_queueItemType.hasValue())
        {
            return new QueueInsertSpecialItemCommand(_queueItemType.value(),
                                                     _index, _indexType);
        }
        else
        {
            return new QueueInsertTrackCommand(_hash, _index, _indexType);
        }
    }

    /* ===== QueueDeleteCommand ===== */

    QueueDeleteCommand::QueueDeleteCommand(quint32 queueId)
     : _queueId(queueId),
       _wasDeleted(false)
    {
        //
    }

    bool QueueDeleteCommand::requiresAuthentication() const
    {
        return true;
    }

    void QueueDeleteCommand::run(ServerInterface* serverInterface)
    {
        auto* queueController = &serverInterface->queueController();

        connect(
            queueController, &QueueController::queueEntryRemoved,
            this,
            [this](qint32 index, quint32 queueId)
            {
                Q_UNUSED(index)

                if (queueId != _queueId)
                    return; /* not the one we deleted */

                _wasDeleted = true;
                listenerSlot();
            }
        );

        addStep(
            [this]() -> StepResult
            {
                if (_wasDeleted)
                    return StepResult::commandSuccessful();

                return StepResult::stepIncomplete();
            }
        );

        queueController->deleteQueueEntry(_queueId);
    }

    /* ===== QueueMoveCommand ===== */

    QueueMoveCommand::QueueMoveCommand(quint32 queueId, qint16 moveOffset)
     : _queueId(queueId),
       _moveOffset(moveOffset),
       _wasMoved(false)
    {
        //
    }

    bool QueueMoveCommand::requiresAuthentication() const
    {
        return true;
    }

    void QueueMoveCommand::run(ServerInterface* serverInterface)
    {
        auto* queueController = &serverInterface->queueController();

        connect(
            queueController, &QueueController::queueEntryMoved,
            this,
            [this](qint32 fromIndex, qint32 toIndex, quint32 queueId)
            {
                int movedOffset = toIndex - fromIndex;

                if (queueId != _queueId || movedOffset != _moveOffset)
                    return; /* not the move we asked for */

                _wasMoved = true;
                listenerSlot();
            }
        );

        addStep(
            [this]() -> StepResult
            {
                if (_wasMoved)
                    return StepResult::commandSuccessful();

                return StepResult::stepIncomplete();
            }
        );

        queueController->moveQueueEntry(_queueId, _moveOffset);
    }
}
