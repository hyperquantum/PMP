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

#include "hashidregistrar.h"
#include "history.h"
#include "users.h"

#include <QRandomGenerator>
#include <QtDebug>
#include <QTimer>

namespace PMP
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

    HistoryStatisticsPrefetcher::HistoryStatisticsPrefetcher(QObject* parent,
                                                         HashIdRegistrar* hashIdRegistrar,
                                                         History* history,
                                                         Users* users)
     : QObject(parent),
       _hashIdRegistrar(hashIdRegistrar),
       _history(history),
       _users(users),
       _timer(new QTimer(this)),
       _workThrottle(new WorkThrottle(this, 5))
    {
        connect(_timer, &QTimer::timeout,
                this, &HistoryStatisticsPrefetcher::doSomething);

        connect(_workThrottle, &WorkThrottle::readyForExtraJob,
                this, &HistoryStatisticsPrefetcher::doSomething);

        _timer->start(1000);
    }

    void HistoryStatisticsPrefetcher::doSomething()
    {
        auto jobCreator =
            [this]() -> Future<SuccessType, FailureType>
            {
                if (_index < 0)
                {
                    prepareList();
                    return FutureResult(success);
                }

                if (_index < _hashIds.size())
                {
                    auto hashId = _hashIds[_index];
                    _index++;

                    return
                        _history->scheduleUserStatsFetchingIfMissing(hashId, /*user*/ 0);
                }

                qDebug() << "HistoryStatisticsPrefetcher: prefetch is complete";
                _finished = true;
                _timer->stop();
                return FutureResult(success);
            };

        if (!_finished)
        {
            _workThrottle->tryStartJob(jobCreator);
        }
    }

    void HistoryStatisticsPrefetcher::prepareList()
    {
        _hashIds = _hashIdRegistrar->getAllIdsLoaded();

        if (_hashIds.isEmpty())
        {
            qDebug() << "HistoryStatisticsPrefetcher: hash list is empty";

            _index = -1;
            _timer->setInterval(qMin(10 * 60 * 1000 /* 10 min */,
                                     _timer->interval() * 2));
            return;
        }

        qDebug() << "HistoryStatisticsPrefetcher: hash list size is"
                 << _hashIds.size();

        _index = 0;
        _timer->setInterval(200);
    }
}
