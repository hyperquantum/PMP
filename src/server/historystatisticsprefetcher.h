/*
    Copyright (C) 2022, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_HISTORYSTATISTICSPREFETCHER_H
#define PMP_HISTORYSTATISTICSPREFETCHER_H

#include "common/future.h"

#include <QMutex>
#include <QObject>
#include <QVector>

QT_FORWARD_DECLARE_CLASS(QTimer)

namespace PMP
{
    class HashIdRegistrar;
    class History;
    class Users;

    class WorkThrottle : public QObject
    {
        Q_OBJECT
    public:
        WorkThrottle(QObject* parent, int maxJobsCount);

        void tryStartJob(std::function<Future<SuccessType, FailureType> ()> jobCreator);

    Q_SIGNALS:
        void readyForExtraJob();

    private:
        void onJobFinished();

        QMutex _mutex;
        int _maxCount;
        int _currentCount;
    };

    class HistoryStatisticsPrefetcher : public QObject
    {
        Q_OBJECT
    public:
        HistoryStatisticsPrefetcher(QObject* parent, HashIdRegistrar* hashIdRegistrar,
                                    History* history, Users* users);

        void start();

    private Q_SLOTS:
        void doSomething();

    private:
        enum class State { Initial, UsersLoading, UsersLoaded, Working, AllDone };

        Future<SuccessType, FailureType> startLoadingUsers();
        void prepareHashesList();
        Future<SuccessType, FailureType> fetchNextStatistics();

        void doubleTimerInterval();

        HashIdRegistrar* _hashIdRegistrar;
        History* _history;
        Users* _users;
        QTimer* _timer;
        WorkThrottle* _workThrottle;
        QVector<uint> _hashIds;
        QVector<uint> _userIds;
        int _hashIndex { -1 };
        int _userIndex { -1 };
        State _state { State::Initial };
    };
}
#endif
