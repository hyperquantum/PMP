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

#ifndef PMP_FUTURE_H
#define PMP_FUTURE_H

#include "concurrentinternals.h"
#include "futureinternals.h"
#include "resultorerror.h"

#include <QSharedPointer>
#include <QtDebug>

#include <functional>

namespace PMP
{
    template<class ResultType, class ErrorType>
    class Future
    {
    public:
        void addListener(std::function<void (ResultOrError<ResultType, ErrorType>)> f)
        {
            _storage->addListener(f);
        }

        void addListener(QObject* receiver,
                         std::function<void (ResultOrError<ResultType, ErrorType>)> f)
        {
            _storage->addListener(receiver, f);
        }

        void addResultListener(QObject* receiver, std::function<void (ResultType)> f)
        {
            _storage->addResultListener(receiver, f);
        }

        void addFailureListener(QObject* receiver, std::function<void (ErrorType)> f)
        {
            _storage->addFailureListener(receiver, f);
        }

        ResultOrError<ResultType, ErrorType> resultOrError()
        {
            return _storage->getResultOrError();
        }

        template<class ResultType2, class ErrorType2>
        Future<ResultType2, ErrorType2> thenFuture(
                std::function<Future<ResultType2, ErrorType2> (
                                                 ResultOrError<ResultType, ErrorType>)> f)
        {
            auto newStorage = FutureStorage<ResultType2, ErrorType2>::create();

            _storage->addListener(
                [newStorage, f](ResultOrError<ResultType, ErrorType> originalOutcome)
                {
                    auto innerFuture = f(originalOutcome);
                    innerFuture.addListener(
                        [newStorage](ResultOrError<ResultType2, ErrorType2> secondResult)
                        {
                            newStorage->setOutcome(secondResult);
                        }
                    );
                }
            );

            return Future<ResultType2, ErrorType2>(newStorage);
        }

        template<class ResultType2, class ErrorType2>
        Future<ResultType2, ErrorType2> thenFuture(
                std::function<Future<ResultType2, ErrorType2> (ResultType)> continuation,
                std::function<ErrorType2 (ErrorType)> errorTranslation)
        {
            auto newStorage = FutureStorage<ResultType2, ErrorType2>::create();

            _storage->addFailureListener(
                [newStorage, errorTranslation](ErrorType error)
                {
                    newStorage->setError(errorTranslation(error));
                }
            );

            _storage->addResultListener(
                [newStorage, continuation](ResultType originalResult)
                {
                    auto innerFuture = continuation(originalResult);
                    innerFuture.addListener(
                        [newStorage](ResultOrError<ResultType2, ErrorType2> secondResult)
                        {
                            newStorage->setOutcome(secondResult);
                        }
                    );
                }
            );

            return Future<ResultType2, ErrorType2>(newStorage);
        }

        template<class ResultType2, class ErrorType2>
        Future<ResultType2, ErrorType2> thenAsync(
                std::function<ResultOrError<ResultType2, ErrorType2> (
                                      ResultOrError<ResultType, ErrorType>)> continuation)
        {
            auto newStorage = FutureStorage<ResultType2, ErrorType2>::create();

            _storage->addListener(
                [newStorage, continuation](
                                     ResultOrError<ResultType, ErrorType> originalOutcome)
                {
                    ConcurrentInternals::run(newStorage, continuation, originalOutcome);
                }
            );

            return Future<ResultType2, ErrorType2>(newStorage);
        }

        template<class ResultType2, class ErrorType2>
        Future<ResultType2, ErrorType2> thenAsync(
                std::function<ResultOrError<ResultType2, ErrorType2> (
                                                                ResultType)> continuation,
                std::function<ErrorType2 (ErrorType)> errorTranslation)
        {
            auto newStorage = FutureStorage<ResultType2, ErrorType2>::create();

            _storage->addFailureListener(
                [newStorage, errorTranslation](ErrorType error)
                {
                    newStorage->setError(errorTranslation(error));
                }
            );

            _storage->addResultListener(
                [newStorage, continuation](ResultType originalResult)
                {
                    auto work =
                        [continuation, originalResult]()
                        {
                            return continuation(originalResult);
                        };

                    ConcurrentInternals::run<ResultType2, ErrorType2>(newStorage, work);
                }
            );

            return Future<ResultType2, ErrorType2>(newStorage);
        }

        template<class ErrorType2>
        Future<ResultType, ErrorType2> convertError(
                                    std::function<ErrorType2 (ErrorType)> errorConversion)
        {
            auto newStorage = FutureStorage<ResultType, ErrorType2>::create();

            _storage->addFailureListener(
                [newStorage, errorConversion](ErrorType error)
                {
                    newStorage->setError(errorConversion(error));
                }
            );

            _storage->addResultListener(
                [newStorage](ResultType result)
                {
                    newStorage->setResult(result);
                }
            );

            return Future<ResultType, ErrorType2>(newStorage);
        }

        static Future fromResult(ResultType result)
        {
            auto storage { FutureStorage<ResultType, ErrorType>::create() };
            storage->setResult(result);
            return Future(storage);
        }

        static Future fromError(ErrorType error)
        {
            auto storage { FutureStorage<ResultType, ErrorType>::create() };
            storage->setError(error);
            return Future(storage);
        }

    private:
        Future(QSharedPointer<FutureStorage<ResultType, ErrorType>> storage)
         : _storage(storage)
        {
            //
        }

        template<class T1, class T2> friend class Future;
        friend class Promise<ResultType, ErrorType>;
        friend class Concurrent;

        QSharedPointer<FutureStorage<ResultType, ErrorType>> _storage;
    };

    template<class T>
    class SimpleFuture
    {
    public:
        void addResultListener(QObject* receiver, std::function<void (T)> f)
        {
            _storage->addResultListener(receiver, f);
        }

        static SimpleFuture fromResult(T result)
        {
            auto storage { FutureStorage<T, FailureType>::create() };
            storage->setResult(result);
            return SimpleFuture(storage);
        }

    private:
        SimpleFuture(QSharedPointer<FutureStorage<T, FailureType>> storage)
         : _storage(storage)
        {
            //
        }

        friend class SimplePromise<T>;

        QSharedPointer<FutureStorage<T, FailureType>> _storage;
    };

    class VoidFuture
    {
    public:
        void addFinishedListener(QObject* receiver, std::function<void ()> f)
        {
            _storage->addResultListener(receiver, [f](SuccessType) { f(); });
        }

        static VoidFuture fromFinished()
        {
            auto storage { FutureStorage<SuccessType, FailureType>::create() };
            storage->setResult(success);
            return VoidFuture(storage);
        }

    private:
        VoidFuture(QSharedPointer<FutureStorage<SuccessType, FailureType>> storage)
         : _storage(storage)
        {
            //
        }

        friend class VoidPromise;

        QSharedPointer<FutureStorage<SuccessType, FailureType>> _storage;
    };
}
#endif
