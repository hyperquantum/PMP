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
#include <QTimer>
#include <QVector>

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

    template<class T>
    struct FutureListener
    {
        QObject* receiver;
        std::function<void (T)> functionToCall;

        FutureListener(QObject* receiver, std::function<void (T)> functionToCall)
         : receiver(receiver), functionToCall(functionToCall)
        {
            //
        }
    };

    template<class ResultType, class ErrorType>
    class FutureStorage
    {
    private:
        FutureStorage()
         : _finished(false)
        {
            //
        }

        void setResult(ResultType result)
        {
            QMutexLocker lock(&_mutex);

            if (_finished)
                return; // not supposed to be called

            _finished = true;
            _result = result;

            for (auto const& listener : _resultListeners)
            {
                auto f = listener.functionToCall;
                QTimer::singleShot(0, listener.receiver, [f, result]() { f(result); });
            }
        }

        void setError(ErrorType error)
        {
            QMutexLocker lock(&_mutex);

            if (_finished)
                return; // not supposed to be called

            _finished = true;
            _error = error;

            for (auto const& listener : _failureListeners)
            {
                auto f = listener.functionToCall;
                QTimer::singleShot(0, listener.receiver, [f, error]() { f(error); });
            }
        }

        void addResultListener(QObject* receiver, std::function<void (ResultType)> f)
        {
            QMutexLocker lock(&_mutex);

            if (!_finished)
            {
                _resultListeners.append(FutureListener<ResultType>(receiver, f));
                return;
            }

            if (_result.hasValue())
            {
                auto value = _result.value();
                QTimer::singleShot(0, receiver, [f, value]() { f(value); });
            }
        }

        void addFailureListener(QObject* receiver, std::function<void (ErrorType)> f)
        {
            QMutexLocker lock(&_mutex);

            if (!_finished)
            {
                _failureListeners.append(FutureListener<ErrorType>(receiver, f));
                return;
            }

            if (_error.hasValue())
            {
                auto error = _error.value();
                QTimer::singleShot(0, receiver, [f, error]() { f(error); });
            }
        }

        friend class Promise<ResultType, ErrorType>;
        friend class Promise<ResultType, void>;
        friend class Promise<void, ErrorType>;
        friend class Promise<void, void>;
        friend class SimplePromise<ResultType>;
        friend class VoidPromise;

        friend class Future<ResultType, ErrorType>;
        friend class Future<ResultType, void>;
        friend class Future<void, ErrorType>;
        friend class Future<void, void>;
        friend class SimpleFuture<ResultType>;
        friend class VoidFuture;

        friend class QSharedPointer<FutureStorage<ResultType, ErrorType>>;

    private:
        QMutex _mutex;
        bool _finished;
        Nullable<ResultType> _result;
        Nullable<ErrorType> _error;
        QVector<FutureListener<ResultType>> _resultListeners;
        QVector<FutureListener<ErrorType>> _failureListeners;
    };

    template<class ResultType, class ErrorType>
    class Future
    {
    public:
        void addResultListener(QObject* receiver, std::function<void (ResultType)> f)
        {
            _storage->addResultListener(receiver, f);
        }

        void addFailureListener(QObject* receiver, std::function<void (ErrorType)> f)
        {
            _storage->addFailureListener(receiver, f);
        }

        static Future fromResult(ResultType result)
        {
            auto storage {QSharedPointer<FutureStorage<ResultType, ErrorType>>::create()};
            storage->setResult(result);
            return Future(storage);
        }

        static Future fromError(ErrorType error)
        {
            auto storage {QSharedPointer<FutureStorage<ResultType, ErrorType>>::create()};
            storage->setError(error);
            return Future(storage);
        }

    private:
        Future(QSharedPointer<FutureStorage<ResultType, ErrorType>> storage)
         : _storage(storage)
        {
            //
        }

        friend class Promise<ResultType, ErrorType>;

        QSharedPointer<FutureStorage<ResultType, ErrorType>> _storage;
    };

    template<class ResultType>
    class Future<ResultType, void>
    {
    public:
        void addResultListener(QObject* receiver, std::function<void (ResultType)> f)
        {
            _storage->addResultListener(receiver, f);
        }

        void addFailureListener(QObject* receiver, std::function<void ()> f)
        {
            _storage->addFailureListener(receiver, [f](int) { f(); });
        }

        static Future fromResult(ResultType result)
        {
            auto storage {QSharedPointer<FutureStorage<ResultType, int>>::create()};
            storage->setResult(result);
            return Future(storage);
        }

        static Future fromError()
        {
            return Future { failure };
        }

        Future(FailureType)
         : _storage(QSharedPointer<FutureStorage<ResultType, int>>::create())
        {
            _storage->setError(0);
        }

    private:
        Future(QSharedPointer<FutureStorage<ResultType, int>> storage)
         : _storage(storage)
        {
            //
        }

        friend class Promise<ResultType, void>;

        QSharedPointer<FutureStorage<ResultType, int>> _storage;
    };

    template<class ErrorType>
    class Future<void, ErrorType>
    {
    public:
        void addSuccessListener(QObject* receiver, std::function<void ()> f)
        {
            _storage->addResultListener(receiver, [f](int) { f(); });
        }

        void addFailureListener(QObject* receiver, std::function<void (ErrorType)> f)
        {
            _storage->addFailureListener(receiver, f);
        }

        static Future fromSuccess()
        {
            return Future { success };
        }

        static Future fromError(ErrorType error)
        {
            auto storage {QSharedPointer<FutureStorage<int, ErrorType>>::create()};
            storage->setError(error);
            return Future(storage);
        }

        Future(SuccessType)
         : _storage(QSharedPointer<FutureStorage<int, ErrorType>>::create())
        {
            _storage->setResult(0);
        }

    private:
        Future(QSharedPointer<FutureStorage<int, ErrorType>> storage)
         : _storage(storage)
        {
            //
        }

        friend class Promise<void, ErrorType>;

        QSharedPointer<FutureStorage<int, ErrorType>> _storage;
    };

    template<>
    class Future<void, void>
    {
    public:
        void addSuccessListener(QObject* receiver, std::function<void ()> f)
        {
            _storage->addResultListener(receiver, [f](int) { f(); });
        }

        void addFailureListener(QObject* receiver, std::function<void ()> f)
        {
            _storage->addFailureListener(receiver, [f](int) { f(); });
        }

        static Future fromSuccess()
        {
            return Future { success };
        }

        static Future fromError()
        {
            return Future { failure };
        }

        Future(SuccessType)
         : _storage(QSharedPointer<FutureStorage<int, int>>::create())
        {
            _storage->setResult(0);
        }

        Future(FailureType)
         : _storage(QSharedPointer<FutureStorage<int, int>>::create())
        {
            _storage->setError(0);
        }

    private:
        Future(QSharedPointer<FutureStorage<int, int>> storage)
         : _storage(storage)
        {
            //
        }

        friend class Promise<void, void>;

        QSharedPointer<FutureStorage<int, int>> _storage;
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
            auto storage {QSharedPointer<FutureStorage<T, int>>::create()};
            storage->setResult(result);
            return SimpleFuture(storage);
        }

    private:
        SimpleFuture(QSharedPointer<FutureStorage<T, int>> storage)
         : _storage(storage)
        {
            //
        }

        friend class SimplePromise<T>;

        QSharedPointer<FutureStorage<T, int>> _storage;
    };

    class VoidFuture
    {
    public:
        void addFinishedListener(QObject* receiver, std::function<void ()> f)
        {
            _storage->addResultListener(receiver, [f](int) { f(); });
        }

        static VoidFuture fromFinished()
        {
            auto storage {QSharedPointer<FutureStorage<int, int>>::create()};
            storage->setResult(0);
            return VoidFuture(storage);
        }

    private:
        VoidFuture(QSharedPointer<FutureStorage<int, int>> storage)
         : _storage(storage)
        {
            //
        }

        friend class VoidPromise;

        QSharedPointer<FutureStorage<int, int>> _storage;
    };
}
#endif
