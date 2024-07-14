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
    template<class T>
    class FutureResult
    {
    public:
        FutureResult(T const& result) : _result(result) {}
        FutureResult(T&& result) : _result(result) {}

    private:
        template<class T1, class T2> friend class Future;
        friend class SimpleFuture<T>;

        T _result;
    };

    template<class T>
    class FutureError
    {
    public:
        FutureError(T const& error) : _error(error) {}
        FutureError(T&& error) : _error(error) {}

    private:
        template<class T1, class T2> friend class Future;

        T _error;
    };

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

        Nullable<ResultOrError<ResultType, ErrorType>> resultOrErrorIfFinished()
        {
            return _storage->getResultOrErrorIfFinished();
        }

        ResultOrError<ResultType, ErrorType> resultOrError()
        {
            return _storage->getResultOrError();
        }

        Future<SuccessType, FailureType> toTypelessFuture()
        {
            auto newStorage = FutureStorage<SuccessType, FailureType>::create();

            _storage->addListener(
                [newStorage](ResultOrError<ResultType, ErrorType> originalOutcome)
                {
                    if (originalOutcome.succeeded())
                        newStorage->setOutcome(success);
                    else
                        newStorage->setOutcome(failure);
                }
            );

            return Future<SuccessType, FailureType>(newStorage);
        }

        template<class T>
        SimpleFuture<T> toSimpleFuture(
            std::function<T (ResultType)> resultConversion,
            std::function<T (ErrorType)> errorConversion)
        {
            auto newStorage = FutureStorage<T, FailureType>::create();

            _storage->addListener(
                [newStorage, resultConversion, errorConversion](
                    ResultOrError<ResultType, ErrorType> originalOutcome)
                {
                    if (originalOutcome.succeeded())
                        newStorage->setOutcome(resultConversion(originalOutcome.result()));
                    else
                        newStorage->setOutcome(errorConversion(originalOutcome.error()));
                }
            );

            return SimpleFuture<T>(newStorage);
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

        template<class ResultType2>
        Future<ResultType2, ErrorType> convertResult(
                                 std::function<ResultType2 (ResultType)> resultConversion)
        {
            auto newStorage = FutureStorage<ResultType2, ErrorType>::create();

            _storage->addResultListener(
                [newStorage, resultConversion](ResultType result)
                {
                    newStorage->setResult(resultConversion(result));
                }
            );

            _storage->addFailureListener(
                [newStorage](ErrorType error)
                {
                    newStorage->setError(error);
                }
            );

            return Future<ResultType2, ErrorType>(newStorage);
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

        Future(FutureResult<ResultType>&& result)
         : _storage { FutureStorage<ResultType, ErrorType>::create() }
        {
            _storage->setResult(result._result);
        }

        static Future fromResult(ResultType result)
        {
            auto storage { FutureStorage<ResultType, ErrorType>::create() };
            storage->setResult(result);
            return Future(storage);
        }

        Future(FutureError<ErrorType>&& error)
         : _storage { FutureStorage<ResultType, ErrorType>::create() }
        {
            _storage->setError(error._error);
        }

        static Future fromError(ErrorType error)
        {
            auto storage { FutureStorage<ResultType, ErrorType>::create() };
            storage->setError(error);
            return Future(storage);
        }

        static Future fromResultOrError(
                ResultOrError<ResultType, ErrorType> resultOrError)
        {
            auto storage { FutureStorage<ResultType, ErrorType>::create() };

            if (resultOrError.succeeded())
                storage->setResult(resultOrError.result());
            else
                storage->setError(resultOrError.error());

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

        SimpleFuture(FutureResult<T>&& result)
         : _storage { FutureStorage<T, FailureType>::create() }
        {
            _storage->setResult(result._result);
        }

    private:
        SimpleFuture(QSharedPointer<FutureStorage<T, FailureType>> storage)
         : _storage(storage)
        {
            //
        }

        template<class T1, class T2> friend class Future;
        friend class SimplePromise<T>;

        QSharedPointer<FutureStorage<T, FailureType>> _storage;
    };
}
#endif
