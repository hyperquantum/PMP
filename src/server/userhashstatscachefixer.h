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

#ifndef PMP_USERHASHSTATSCACHEFIXER_H
#define PMP_USERHASHSTATSCACHEFIXER_H

#include "common/resultorerror.h"

#include <QHash>
#include <QObject>
#include <QSet>

QT_FORWARD_DECLARE_CLASS(QTimer)

namespace PMP::Server
{
    class Database;
    class HistoryStatistics;

    class UserHashStatsCacheFixer : public QObject
    {
        Q_OBJECT
    public:
        UserHashStatsCacheFixer(HistoryStatistics* historyStatistics);

        void start();

    private:
        enum class State
        {
            Initial,
            WaitBeforeDeciding,
            DecideWhatToDo,
            ProcessingHistory,
            Finished
        };

        void work();
        void handleResultOfWork(SuccessOrFailure result);
        void setStateToWaitBeforeDeciding(uint waitTimeMs);
        SuccessOrFailure decideWhatToDo();
        SuccessOrFailure fetchHistoryIdFromMiscData(Database& database);
        SuccessOrFailure processHistory();

        HistoryStatistics* _historyStatistics;
        State _state { State::Initial };
        uint _waitingTimeMs {0};
        QString _oldHistoryIdString;
        uint _oldHistoryId {0};
        int _historyCountToProcess {0};
        QHash<quint32, QSet<uint>> _usersWithHashesAlreadyInvalidated;
    };
}
#endif
