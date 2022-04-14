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

#include "nullable.h"
#include "resultorerror.h"

#include <QMutex>
#include <QMutexLocker>
#include <QSharedPointer>
#include <QtDebug>
#include <QtGlobal>
#include <QTimer>
#include <QVector>
#include <QWaitCondition>

#include <functional>

namespace PMP
{
    template<class ResultType, class ErrorType>
    class Future;

    template<class T>
    class SimpleFuture;

    class VoidFuture;

    template<class ResultType, class ErrorType>
    class Promise;

    template<class T>
    class SimplePromise;

    class VoidPromise;

    template<class ResultType, class ErrorType>
    class FutureStorage
    {
    private:
        FutureStorage()
         : _finished(false)
        {
            //
        }

        static QSharedPointer<FutureStorage<ResultType, ErrorType>> create()
        {
            return QSharedPointer<FutureStorage<ResultType, ErrorType>>::create();
        }

        void setOutcome(ResultOrError<ResultType, ErrorType> outcome)
        {
            if (outcome.succeeded())
                setResult(outcome.result());
            else
                setError(outcome.error());
        }

        void setResult(ResultType result)
        {
            QMutexLocker lock(&_mutex);

            Q_ASSERT_X(!_finished, "FutureStorage::setResult()",
                       "attempt to set result on finished future");

            _finished = true;
            _result = result;

            for (auto const& listener : _resultListeners)
                listener(result);

            _waitCondition.wakeAll();
        }

        void setError(ErrorType error)
        {
            QMutexLocker lock(&_mutex);

            Q_ASSERT_X(!_finished, "FutureStorage::setError()",
                       "attempt to set error on finished future");

            _finished = true;
            _error = error;

            for (auto const& listener : _failureListeners)
                listener(error);

            _waitCondition.wakeAll();
        }

        void addListener(std::function<void (ResultOrError<ResultType, ErrorType>)> f)
        {
            QMutexLocker lock(&_mutex);

            if (!_finished)
            {
                _resultListeners.append(
                    [f](ResultType result)
                    {
                        f(ResultOrError<ResultType, ErrorType>::fromResult(result));
                    }
                );
                _failureListeners.append(
                    [f](ErrorType error)
                    {
                        f(ResultOrError<ResultType, ErrorType>::fromError(error));
                    }
                );
                return;
            }

            if (_result.hasValue())
            {
                auto value = _result.value();
                f(ResultOrError<ResultType, ErrorType>::fromResult(value));
            }
            else
            {
                auto error = _error.value();
                f(ResultOrError<ResultType, ErrorType>::fromError(error));
            }
        }

        void addListener(QObject* receiver,
                         std::function<void (ResultOrError<ResultType, ErrorType>)> f)
        {
            auto listener =
                [receiver, f](ResultOrError<ResultType, ErrorType> result)
                {
                    QTimer::singleShot(0, receiver, [f, result]() { f(result); });
                };

            addListener(listener);
        }

        void addResultListener(std::function<void (ResultType)> f)
        {
            QMutexLocker lock(&_mutex);

            if (!_finished)
            {
                _resultListeners.append(f);
                return;
            }

            if (_result.hasValue())
            {
                auto value = _result.value();
                f(value);
            }
        }

        void addResultListener(QObject* receiver, std::function<void (ResultType)> f)
        {
            auto listener =
                [receiver, f](ResultType result)
                {
                    QTimer::singleShot(0, receiver, [f, result]() { f(result); });
                };

            addResultListener(listener);
        }

        void addFailureListener(std::function<void (ErrorType)> f)
        {
            QMutexLocker lock(&_mutex);

            if (!_finished)
            {
                _failureListeners.append(f);
                return;
            }

            if (_error.hasValue())
            {
                auto error = _error.value();
                f(error);
            }
        }

        void addFailureListener(QObject* receiver, std::function<void (ErrorType)> f)
        {
            auto listener =
                [receiver, f](ErrorType error)
                {
                    QTimer::singleShot(0, receiver, [f, error]() { f(error); });
                };

            addFailureListener(listener);
        }

        ResultOrError<ResultType, ErrorType> getResultOrError()
        {
            waitUntilFinished();

            QMutexLocker lock(&_mutex);

            if (_result.hasValue())
                return ResultOrError<ResultType, ErrorType>::fromResult(_result.value());
            else
                return ResultOrError<ResultType, ErrorType>::fromError(_error.value());
        }

        void waitUntilFinished()
        {
            QMutexLocker lock(&_mutex);

            while (!_finished)
                _waitCondition.wait(&_mutex);
        }

    private:
        friend class Promise<ResultType, ErrorType>;
        friend class SimplePromise<ResultType>;
        friend class VoidPromise;

        template<class T1, class T2> friend class Future;
        friend class SimpleFuture<ResultType>;
        friend class VoidFuture;

        friend class QSharedPointer<FutureStorage<ResultType, ErrorType>>;

        QMutex _mutex;
        QWaitCondition _waitCondition;
        bool _finished;
        Nullable<ResultType> _result;
        Nullable<ErrorType> _error;
        QVector<std::function<void (ResultType)>> _resultListeners;
        QVector<std::function<void (ErrorType)>> _failureListeners;
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

            _storage->addListener(
                [newStorage, continuation, errorTranslation](
                                     ResultOrError<ResultType, ErrorType> originalOutcome)
                {
                    if (originalOutcome.failed())
                    {
                        newStorage->setError(errorTranslation(originalOutcome.error()));
                        return;
                    }

                    auto innerFuture = continuation(originalOutcome.result());
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
