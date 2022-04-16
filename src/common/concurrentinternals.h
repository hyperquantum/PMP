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

#ifndef PMP_CONCURRENTINTERNALS_H
#define PMP_CONCURRENTINTERNALS_H

#include "futureinternals.h"

#include <QAtomicInteger>
#include <QtConcurrent/QtConcurrent>

#include <functional>

namespace PMP
{
    class Concurrent;

    class ConcurrentInternals
    {
    private:
        template<class ResultType, class ErrorType>
        static void run(QSharedPointer<FutureStorage<ResultType, ErrorType>> storage,
                        std::function<ResultOrError<ResultType, ErrorType> ()> f)
        {
            QtConcurrent::run(makeWork(storage, f));
        }

        template<class ResultType, class ErrorType>
        static void run(QSharedPointer<FutureStorage<ResultType, ErrorType>> storage,
                        QThreadPool* threadPool,
                        std::function<ResultOrError<ResultType, ErrorType> ()> f)
        {
            QtConcurrent::run(threadPool, makeWork(storage, f));
        }

        template<class ResultType, class ErrorType, class ResultType2, class ErrorType2>
        static void run(QSharedPointer<FutureStorage<ResultType, ErrorType>> storage,
                        std::function<ResultOrError<ResultType, ErrorType> (
                                               ResultOrError<ResultType2, ErrorType2>)> f,
                        ResultOrError<ResultType2, ErrorType2> arg)
        {
            QtConcurrent::run(makeWork(storage, f, arg));
        }

        template<class ResultType, class ErrorType, class ResultType2, class ErrorType2>
        static void run(QSharedPointer<FutureStorage<ResultType, ErrorType>> storage,
                        QThreadPool* threadPool,
                        std::function<ResultOrError<ResultType, ErrorType> (
                                               ResultOrError<ResultType2, ErrorType2>)> f,
                        ResultOrError<ResultType2, ErrorType2> arg)
        {
            QtConcurrent::run(threadPool, makeWork(storage, f, arg));
        }

        static void waitUntilEverythingFinished();

        class CountIncrementer
        {
        public:
            CountIncrementer()
             : _count(&_runningCount)
            {
                _count->ref();
            }

            ~CountIncrementer()
            {
                _count->deref();
            }

        private :
            QAtomicInteger<int>* _count;
        };

    private:
        template<class ResultType, class ErrorType>
        static std::function<void ()> makeWork(
                            QSharedPointer<FutureStorage<ResultType, ErrorType>> storage,
                            std::function<ResultOrError<ResultType, ErrorType> ()> f)
        {
            auto work =
                [storage, f]()
                {
                    CountIncrementer countIncrement;
                    auto outcome = f();
                    storage->setOutcome(outcome);
                };

            return work;
        }

        template<class ResultType, class ErrorType, class ResultType2, class ErrorType2>
        static std::function<void ()> makeWork(
                    QSharedPointer<FutureStorage<ResultType, ErrorType>> storage,
                    std::function<ResultOrError<ResultType, ErrorType> (
                                               ResultOrError<ResultType2, ErrorType2>)> f,
                    ResultOrError<ResultType2, ErrorType2> arg)
        {
            auto work =
                [storage, f, arg]()
                {
                    CountIncrementer countIncrement;
                    auto outcome = f(arg);
                    storage->setOutcome(outcome);
                };

            return work;
        }

        friend class Concurrent;
        template<class T1, class T2> friend class Future;

        ConcurrentInternals() {}

        static QAtomicInteger<int> _runningCount;
    };
}
#endif
