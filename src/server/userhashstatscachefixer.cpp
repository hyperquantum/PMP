/*
    Copyright (C) 2022-2024, Kevin Andre <hyperquantum@gmail.com>

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

#include "userhashstatscachefixer.h"

#include "common/newconcurrent.h"

#include "database.h"
#include "historystatistics.h"

#include <QtDebug>
#include <QTimer>

namespace PMP::Server
{
    namespace
    {
        const char* const miscDataKey = "UserHashStatsCacheHistoryId";
    }

    UserHashStatsCacheFixer::UserHashStatsCacheFixer(HistoryStatistics* historyStatistics)
        : _historyStatistics(historyStatistics)
    {
        //
    }

    void UserHashStatsCacheFixer::start()
    {
        Q_ASSERT_X(_state == State::Initial,
                   "UserHashStatsCacheFixer::start()",
                   "state not equal to Initial");

        work();
    }

    void UserHashStatsCacheFixer::work()
    {
        std::function<SuccessOrFailure (void)> workToDo;

        switch (_state)
        {
        case State::Initial:
            setStateToWaitBeforeDeciding(5000);
            work();
            return;

        case State::WaitBeforeDeciding:
            QTimer::singleShot(
                _waitingTimeMs,
                this,
                [this]() { _state = State::DecideWhatToDo; work(); }
            );
            return;

        case State::DecideWhatToDo:
            workToDo = [this]() { return decideWhatToDo(); };
            break;

        case State::ProcessingHistory:
            workToDo = [this]() { return processHistory(); };
            break;

        case State::Finished:
            qDebug() << "UserHashStatsCacheFixer: finished";
            return;

        default:
            Q_UNREACHABLE();
        }

        NewConcurrent::runOnThreadPool(globalThreadPool, workToDo)
            .handleOnEventLoop(
                this, [this](SuccessOrFailure result) { handleResultOfWork(result); }
            );
    }

    void UserHashStatsCacheFixer::handleResultOfWork(SuccessOrFailure result)
    {
        if (result.succeeded())
        {
            work();
            return;
        }

        qWarning() << "UserHashStatsCacheFixer: ancountered a problem in state"
                   << int(_state) << "; will try again later";

        setStateToWaitBeforeDeciding(5 * 60 * 1000 /* 5 min */);
        work();
    }

    void UserHashStatsCacheFixer::setStateToWaitBeforeDeciding(uint waitTimeMs)
    {
        qDebug() << "UserHashStatsCacheFixer: going to wait for"
                 << waitTimeMs << "ms before deciding what needs to be done";

        _waitingTimeMs = waitTimeMs;
        _state = State::WaitBeforeDeciding;
    }

    SuccessOrFailure UserHashStatsCacheFixer::decideWhatToDo()
    {
        qDebug() << "UserHashStatsCacheFixer: going to decide what needs to be done";

        auto database = Database::getDatabaseForCurrentThread();
        if (!database)
            return failure;

        auto historyIdFetched = fetchHistoryIdFromMiscData(*database);
        if (historyIdFetched.failed())
            return failure;

        if (_oldHistoryIdString.isEmpty() && _oldHistoryId == 0)
            return success; // we will try again next time

        auto lastHistoryIdOrFailure = database->getLastHistoryId();
        if (lastHistoryIdOrFailure.failed())
            return failure;

        uint lastHistoryId = lastHistoryIdOrFailure.result();

        qDebug() << "UserHashStatsCacheFixer: history IDs:" << _oldHistoryId
                 << "and" << lastHistoryId;

        if (_oldHistoryId >= lastHistoryId)
        {
            _state = State::Finished;
            return success;
        }

        _historyCountToProcess = int(qMin(10u, lastHistoryId - _oldHistoryId));
        _state = State::ProcessingHistory;

        qDebug() << "UserHashStatsCacheFixer: going to process"
                 << _historyCountToProcess << "history items";

        return success;
    }

    SuccessOrFailure UserHashStatsCacheFixer::fetchHistoryIdFromMiscData(
        Database& database)
    {
        _oldHistoryIdString = "";
        _oldHistoryId = 0;

        auto historyIdOrFailure = database.getMiscDataValue(miscDataKey);

        if (historyIdOrFailure.failed())
            return failure;

        Nullable<QString> historyIdAsString = historyIdOrFailure.result();

        if (historyIdAsString == null)
            return database.insertMiscDataIfNotPresent(miscDataKey, "0");

        bool historyIdParsed = false;
        uint historyId = historyIdAsString.value().toUInt(&historyIdParsed);

        if (historyIdParsed)
        {
            _oldHistoryIdString = historyIdAsString.value();
            _oldHistoryId = historyId;
            return success;
        }

        // overwrite the value and then return in order to try again next time
        return database.updateMiscDataValueFromSpecific(miscDataKey,
                                                        historyIdAsString.value(),
                                                        "0");
    }

    SuccessOrFailure UserHashStatsCacheFixer::processHistory()
    {
        qDebug() << "UserHashStatsCacheFixer: going to process history items"
                 << (_oldHistoryId + 1) << "through"
                 << (_oldHistoryId + _historyCountToProcess);

        auto database = Database::getDatabaseForCurrentThread();
        if (!database)
            return failure;

        auto historyRecordsOrFailure =
            database->getBriefHistoryFragment(_oldHistoryId + 1, _historyCountToProcess);

        if (historyRecordsOrFailure.failed())
            return failure;

        const auto historyRecords = historyRecordsOrFailure.result();

        uint lastHistoryIdProcessed = 0;

        for (auto const& historyRecord : historyRecords)
        {
            lastHistoryIdProcessed = historyRecord.id;

            const auto userId = historyRecord.userId;
            const auto hashId = historyRecord.hashId;

            auto& hashesAlreadyInvalidated = _usersWithHashesAlreadyInvalidated[userId];
            bool wasAlreadyInvalidated = hashesAlreadyInvalidated.contains(hashId);

            if (wasAlreadyInvalidated)
                continue;

            _historyStatistics->invalidateIndividualHashStatistics(historyRecord.userId,
                                                                   historyRecord.hashId);

            hashesAlreadyInvalidated << hashId;
        }

        if (lastHistoryIdProcessed <= _oldHistoryId)
            return failure;

        const auto newHistoryIdString = QString::number(lastHistoryIdProcessed);

        auto tryUpdateMiscData =
            database->updateMiscDataValueFromSpecific(miscDataKey,
                                                      _oldHistoryIdString,
                                                      newHistoryIdString);

        if (tryUpdateMiscData.failed())
            return failure;

        setStateToWaitBeforeDeciding(3000);
        return success;
    }
}
