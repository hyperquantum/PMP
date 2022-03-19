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

    template<class ResultType>
    class FutureResult;

    template<class ErrorType>
    class FutureAction;

    template<class ResultType, class ErrorType>
    class Promise;

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

        void setFailed(ErrorType error)
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

        friend class Future<ResultType, ErrorType>;
        friend class FutureResult<ResultType>;
        friend class FutureAction<ErrorType>;

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
    class FutureResult
    {
    public:
        void addResultListener(QObject* receiver, std::function<void (ResultType)> f)
        {
            _storage->addResultListener(receiver, f);
        }

    private:
        FutureResult(QSharedPointer<FutureStorage<ResultType, int>> storage)
         : _storage(storage)
        {
            //
        }

        friend class Promise<ResultType, void>;

        QSharedPointer<FutureStorage<ResultType, int>> _storage;
    };

    template<class ErrorType>
    class FutureAction
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

    private:
        FutureAction(QSharedPointer<FutureStorage<int, ErrorType>> storage)
         : _storage(storage)
        {
            //
        }

        friend class Promise<void, ErrorType>;

        QSharedPointer<FutureStorage<int, ErrorType>> _storage;
    };
}
#endif
