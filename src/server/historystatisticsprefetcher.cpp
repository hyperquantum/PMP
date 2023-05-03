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

#include "historystatisticsprefetcher.h"

#include "common/concurrent.h"

#include "database.h"
#include "hashidregistrar.h"
#include "history.h"
#include "users.h"

#include <QtDebug>
#include <QTimer>

namespace PMP::Server
{
    WorkThrottle::WorkThrottle(QObject* parent, int maxJobsCount)
     : QObject(parent),
       _maxCount(maxJobsCount),
       _currentCount(0)
    {
        if (_maxCount > 0)
        {
            QTimer::singleShot(0, this, &WorkThrottle::readyForExtraJob);
        }
    }

    void WorkThrottle::tryStartJob(
                            std::function<Future<SuccessType, FailureType> ()> jobCreator)
    {
        {
            QMutexLocker lock(&_mutex);

            if (_currentCount >= _maxCount)
                return;

            _currentCount++;
        }

        auto future = jobCreator();
        future.addListener(
            this,
            [this](ResultOrError<SuccessType, FailureType>) { onJobFinished(); }
        );
    }

    void WorkThrottle::onJobFinished()
    {
        {
            QMutexLocker lock(&_mutex);
            _currentCount--;
        }

        Q_EMIT readyForExtraJob();
    }

    static const int maxPrefetchJobs = 2;

    HistoryStatisticsPrefetcher::HistoryStatisticsPrefetcher(QObject* parent,
                                                         HashIdRegistrar* hashIdRegistrar,
                                                         History* history,
                                                         Users* users)
     : QObject(parent),
       _hashIdRegistrar(hashIdRegistrar),
       _history(history),
       _users(users),
       _timer(new QTimer(this)),
       _workThrottle(new WorkThrottle(this, maxPrefetchJobs))
    {
        connect(_timer, &QTimer::timeout,
                this, &HistoryStatisticsPrefetcher::doSomething);

        connect(_workThrottle, &WorkThrottle::readyForExtraJob,
                this, &HistoryStatisticsPrefetcher::doSomething);

        _timer->setInterval(1000);
    }

    void HistoryStatisticsPrefetcher::start()
    {
        if (_timer->isActive())
            return;

        qDebug() << "HistoryStatisticsPrefetcher: starting";
        _timer->start();
    }

    void HistoryStatisticsPrefetcher::doSomething()
    {
        auto jobCreator =
            [this]() -> Future<SuccessType, FailureType>
            {
                switch (_state)
                {
                case State::Initial:
                    return startLoadingUsers();

                case State::UsersLoading:
                    break; /* still waiting */

                case State::UsersLoaded:
                    prepareHashesList();
                    break;

                case State::Working:
                    return fetchNextStatistics();

                case State::AllDone:
                    break;
                }

                return FutureResult(success);
            };

        if (_state != State::AllDone)
        {
            _workThrottle->tryStartJob(jobCreator);
        }
    }

    Future<SuccessType, FailureType> HistoryStatisticsPrefetcher::startLoadingUsers()
    {
        Q_ASSERT_X(_state == State::Initial,
                   "HistoryStatisticsPrefetcher::startLoadingUsers()",
                   "state not equal to Initial");

        _state = State::UsersLoading;

        auto future =
            Concurrent::run<quint32, FailureType>(
                []() -> ResultOrError<quint32, FailureType>
                {
                    auto db = Database::getDatabaseForCurrentThread();
                    if (!db)
                        return failure;

                    return db->getMostRecentRealUserHavingHistory();
                }
            );

        future.addFailureListener(
            this,
            [this](FailureType)
            {
                _state = State::Initial;
                doubleTimerInterval();
            }
        );
        future.addResultListener(
            this,
            [this](quint32 userId)
            {
                if (userId > 0)
                    _userIds = { 0, userId };
                else
                    _userIds = { 0 };

                qDebug() << "HistoryStatisticsPrefetcher: user IDs:" << _userIds;
                _state = State::UsersLoaded;
            }
        );

        return future.toTypelessFuture();
    }

    void HistoryStatisticsPrefetcher::prepareHashesList()
    {
        Q_ASSERT_X(_state == State::UsersLoaded,
                   "HistoryStatisticsPrefetcher::prepareHashesList()",
                   "state not equal to UsersLoaded");

        _hashIds = _hashIdRegistrar->getAllIdsLoaded();

        if (_hashIds.isEmpty())
        {
            qDebug() << "HistoryStatisticsPrefetcher: hash list empty, will wait a bit";

            doubleTimerInterval();
        }
        else
        {
            qDebug() << "HistoryStatisticsPrefetcher: hash list size is"
                     << _hashIds.size();

            _hashIndex = 0;
            _userIndex = 0;
            _state = State::Working;
            _timer->setInterval(200);
        }
    }

    Future<SuccessType, FailureType> HistoryStatisticsPrefetcher::fetchNextStatistics()
    {
        Q_ASSERT_X(_state == State::Working,
                   "HistoryStatisticsPrefetcher::fetchNextStatistics()",
                   "state not equal to Working");

        if (_userIndex >= _userIds.size())
        {
            _hashIndex++;
            _userIndex = 0;
        }

        if (_hashIndex >= _hashIds.size())
        {
            qDebug() << "HistoryStatisticsPrefetcher: prefetch is complete";
            _state = State::AllDone;
            _userIndex = -1;
            _hashIndex = -1;
            _timer->stop();
            return FutureResult(success);
        }

        auto userId = _userIds[_userIndex];
        auto hashId = _hashIds[_hashIndex];
        _userIndex++;

        return _history->scheduleUserStatsFetchingIfMissing(hashId, userId);
    }

    void HistoryStatisticsPrefetcher::doubleTimerInterval()
    {
        _timer->setInterval(qMin(10 * 60 * 1000 /* 10 min */,
                                 _timer->interval() * 2));
    }
}
