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

#ifndef PMP_CONCURRENT_H
#define PMP_CONCURRENT_H

#include "concurrentinternals.h"
#include "future.h"
#include "promise.h"

#include <QtConcurrent/QtConcurrent>

#include <functional>
#include <memory>

namespace PMP
{
    class Concurrent
    {
    public:
        template<class ResultType, class ErrorType>
        static Future<ResultType, ErrorType> run(
                                std::function<ResultOrError<ResultType, ErrorType> ()> f)
        {
            auto storage =
                ConcurrentInternals::createFutureStorage<ResultType, ErrorType>();

            ConcurrentInternals::run(storage, f);
            return Future<ResultType, ErrorType>(storage);
        }

        template<class ResultType, class ErrorType>
        static Future<ResultType, ErrorType> run(
                                QThreadPool* threadPool,
                                std::function<ResultOrError<ResultType, ErrorType> ()> f)
        {
            auto storage =
                ConcurrentInternals::createFutureStorage<ResultType, ErrorType>();

            ConcurrentInternals::run(storage, threadPool, f);
            return Future<ResultType, ErrorType>(storage);
        }

        template<class T>
        static SimpleFuture<T> run(SimplePromise<T>&& promise, std::function<T ()> f)
        {
            auto future = promise.future();
            QtConcurrent::run(makeWork(std::move(promise), f));
            return future;
        }

        template<class T>
        static SimpleFuture<T> run(QThreadPool* threadPool, SimplePromise<T>&& promise,
                                   std::function<T ()> f)
        {
            auto future = promise.future();
            QtConcurrent::run(threadPool, makeWork(std::move(promise), f));
            return future;
        }

    private:
        template<class T>
        static std::function<void ()> makeWork(SimplePromise<T>&& promise,
                                               std::function<T ()> f)
        {
            auto sharedPromise = std::make_shared<SimplePromise<T>>(std::move(promise));

            auto work =
                    [sharedPromise, f]()
                    {
                        auto result = f();
                        sharedPromise->setResult(result);
                    };

            return work;
        }

        Concurrent() {}
    };
}
#endif
