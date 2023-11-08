/*
    Copyright (C) 2023, Kevin Andre <hyperquantum@gmail.com>

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

#include "historycommands.h"

#include "common/util.h"

#include "client/historycontroller.h"
#include "client/queueentryinfostorage.h"
#include "client/serverinterface.h"

using namespace PMP::Client;

namespace PMP
{
    /* ===== HistoryCommand ===== */

    bool HistoryCommand::requiresAuthentication() const
    {
        return true;
    }

    void HistoryCommand::run(Client::ServerInterface* serverInterface)
    {
        auto* historyController = &serverInterface->historyController();
        auto* queueEntryInfoStorage = &serverInterface->queueEntryInfoStorage();

        connect(
            historyController, &HistoryController::receivedPlayerHistory,
            this,
            [this, queueEntryInfoStorage](QVector<PMP::PlayerHistoryTrackInfo> tracks)
            {
                _tracks = tracks;
                _listReceived = true;

                for (auto& track : _tracks)
                {
                    queueEntryInfoStorage->fetchEntry(track.queueID());
                }

                listenerSlot();
            }
        );
        connect(
            queueEntryInfoStorage, &QueueEntryInfoStorage::tracksChanged,
            this, &HistoryCommand::listenerSlot
        );

        addStep(
            [this, queueEntryInfoStorage]() -> StepResult
            {
                if (!_listReceived)
                    return StepResult::stepIncomplete();

                bool needToWaitForFilename = false;
                for (auto& track : _tracks)
                {
                    auto queueId = track.queueID();

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
            [this, queueEntryInfoStorage]() -> StepResult
            {
                return printHistory(queueEntryInfoStorage);
            }
        );

        historyController->sendPlayerHistoryRequest(_fetchLimit);
    }

    CommandBase::StepResult HistoryCommand::printHistory(
        QueueEntryInfoStorage* queueEntryInfoStorage)
    {
        QString output;
        output.reserve(80 + 80 + 80 * _fetchLimit);

        output += "       Ended       |  QID  | Length |"
                  " Title                          | Artist";

        for (int index = 0; index < _tracks.size(); ++index)
        {
            output += "\n";
            auto const& entry = _tracks[index];

            auto endedAt = entry.ended().toLocalTime();
            output += endedAt.toString("yyyy-MM-dd HH:mm:ss");
            output += "|";

            auto queueId = entry.queueID();
            output += QString::number(queueId).rightJustified(7);
            output += "|";

            auto* info = queueEntryInfoStorage->entryInfoByQueueId(entry.queueID());

            auto lengthMilliseconds = info ? info->lengthInMilliseconds() : -1;
            if (lengthMilliseconds >= 0)
            {
                auto lengthString =
                    Util::millisecondsToShortDisplayTimeText(lengthMilliseconds);
                output += lengthString.rightJustified(8);
            }
            else
            {
                output += "   ??   ";
            }
            output += "|";

            if (info == nullptr)
            {
                output += " ??";
            }
            else if (info->needFilename() && !info->informativeFilename().isEmpty())
            {
                output += info->informativeFilename();
            }
            else
            {
                output += info->title().leftJustified(32);
                output += "|";
                output += info->artist();
            }
        }

        return StepResult::commandSuccessful(output);
    }
}
