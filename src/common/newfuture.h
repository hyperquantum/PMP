/*
    Copyright (C) 2024, Kevin André <hyperquantum@gmail.com>

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

#ifndef PMP_COMMON_NEWFUTURE_H
#define PMP_COMMON_NEWFUTURE_H

#include "nullable.h"
#include "resultorerror.h"
#include "runners.h"

// NewFutureStorage
#include <QMutex>
#include <QSharedPointer>

// debug
#include <QtDebug>

namespace PMP
{
    /* ---------- THIS FILE IS AN EXPERIMENTAL WORK IN PROGRESS ---------- */

    template<class TResult, class TError> class NewPromise;
    template<class TResult, class TError> class NewFuture;
    template<class TOutcome> class NewSimplePromise;
    template<class TOutcome> class NewSimpleFuture;
    template<class TResult, class TError> class NewFutureStorage;
    template<class TResult, class TError> class Continuation;

    // =================================================================== //

    template<class TResult, class TError>
    class Continuation
    {
    public:
        Continuation(QSharedPointer<Runner> runner,
                     std::function<void (ResultOrError<TResult, TError>)> work);

        void continueFrom(QSharedPointer<Runner> previousRunner,
                          ResultOrError<TResult, TError> const& previousOutcome);

    private:
        friend class QSharedPointer<Continuation<TResult, TError>>;

        QSharedPointer<Runner> _runner;
        std::function<void (ResultOrError<TResult, TError>)> _work;
    };

    template<class TResult, class TError>
    Continuation<TResult, TError>::Continuation(QSharedPointer<Runner> runner,
                                std::function<void (ResultOrError<TResult, TError>)> work)
        : _runner(runner), _work(work)
    {
        //
    }

    template<class TResult, class TError>
    void Continuation<TResult, TError>::continueFrom(
                                    QSharedPointer<Runner> previousRunner,
                                    ResultOrError<TResult, TError> const& previousOutcome)
    {
        // TODO: continuation in same runner if possible
        Q_UNUSED(previousRunner)

        auto wrapper =
            [work = _work, previousOutcome]()
            {
                work(previousOutcome);
            };

        _runner->run(wrapper);
    }

    // =================================================================== //

    template<class TResult, class TError>
    class NewFutureStorage
    {
    public:
        NewFutureStorage(NewFutureStorage<TResult, TError> const&) = delete;
        NewFutureStorage<TResult, TError>& operator=(NewFutureStorage<TResult, TError> const&) = delete;

    private:
        using ContinuationPtr = QSharedPointer<Continuation<TResult, TError>>;

        NewFutureStorage() {}

        static QSharedPointer<NewFutureStorage<TResult, TError>> create();
        static QSharedPointer<NewFutureStorage<TResult, TError>> createWithResult(
            TResult const& result);
        static QSharedPointer<NewFutureStorage<TResult, TError>> createWithError(
            TError const& error);

        ResultOrError<TResult, TError> getOutcomeInternal() const;

        void setContinuation(ContinuationPtr continuation);

        void storeAndContinueFrom(ResultOrError<TResult, TError> const& outcome,
                                  QSharedPointer<Runner> runner);

        friend class QSharedPointer<NewFutureStorage<TResult, TError>>;
        friend class NewPromise<TResult, TError>;
        friend class NewSimplePromise<TResult>;
        template<class, class> friend class NewFuture;
        friend class NewSimpleFuture<TResult>;
        friend class NewConcurrent;

        QMutex _mutex;
        ContinuationPtr _continuation;
        Nullable<TResult> _result;
        Nullable<TError> _error;
        bool _finished { false };
    };

    template<class TResult, class TError>
    QSharedPointer<NewFutureStorage<TResult, TError>>
        NewFutureStorage<TResult, TError>::create()
    {
        return QSharedPointer<NewFutureStorage<TResult, TError>>::create();
    }

    template<class TResult, class TError>
    QSharedPointer<NewFutureStorage<TResult, TError>>
        NewFutureStorage<TResult, TError>::createWithResult(const TResult& result)
    {
        auto storage = QSharedPointer<NewFutureStorage<TResult, TError>>::create();

        storage->_finished = true;
        storage->_result = result;

        return storage;
    }

    template<class TResult, class TError>
    QSharedPointer<NewFutureStorage<TResult, TError>>
        NewFutureStorage<TResult, TError>::createWithError(TError const& error)
    {
        auto storage = QSharedPointer<NewFutureStorage<TResult, TError>>::create();

        storage->_finished = true;
        storage->_error = error;

        return storage;
    }

    template<class TResult, class TError>
    ResultOrError<TResult, TError>
        NewFutureStorage<TResult, TError>::getOutcomeInternal() const
    {
        Q_ASSERT_X(_finished, "NewFutureStorage::getResultInternal()",
                   "attempt to get result of unfinished future");

        if (_result.hasValue())
            return _result.value();
        else
            return _error.value();
    }

    template<class TResult, class TError>
    void NewFutureStorage<TResult, TError>::setContinuation(ContinuationPtr continuation)
    {
        QMutexLocker lock(&_mutex);

        Q_ASSERT_X(continuation, "NewFutureStorage::setContinuation()",
                   "continuation is nullptr");

        Q_ASSERT_X(!_continuation, "NewFutureStorage::setContinuation()",
                   "attempt to set result on finished future");

        if (_finished)
        {
            // when already finished, continue immediately
            auto outcome = getOutcomeInternal();
            lock.unlock();
            continuation->continueFrom(nullptr, outcome);
        }
        else
        {
            _continuation = continuation;
        }
    }

    template<class TResult, class TError>
    void NewFutureStorage<TResult, TError>::storeAndContinueFrom(
        ResultOrError<TResult, TError> const& outcome, QSharedPointer<Runner> runner)
    {
        QMutexLocker lock(&_mutex);

        Q_ASSERT_X(!_finished, "NewFutureStorage::storeAndContinueFrom()",
                   "attempt to set result on finished future");

        _finished = true;

        if (outcome.succeeded())
            _result = outcome.result();
        else
            _error = outcome.error();

        // TODO: notify listeners

        auto continuation = _continuation;

        lock.unlock(); /* unlock before continuing */

        if (continuation)
        {
            continuation->continueFrom(runner, outcome);
        }
    }

    // =================================================================== //

    template<class TResult>
    class NewFutureResult
    {
    public:
        NewFutureResult(TResult const& result) : _result(result) {}
        NewFutureResult(TResult&& result) : _result(result) {}

    private:
        template<class T1, class T2> friend class NewFuture;
        friend class NewSimpleFuture<TResult>;

        TResult _result;
    };

    template<class TError>
    class NewFutureError
    {
    public:
        NewFutureError(TError const& error) : _error(error) {}
        NewFutureError(TError&& error) : _error(error) {}

    private:
        template<class T1, class T2> friend class NewFuture;

        TError _error;
    };

    // =================================================================== //

    template<class TResult, class TError>
    class NewFuture
    {
    public:
        using OutcomeType = ResultOrError<TResult, TError>;

        NewFuture(NewFutureResult<TResult> const& result);
        NewFuture(NewFutureError<TError> const& error);

        template<class TResult2, class TError2>
        NewFuture<TResult2, TError2> thenOnThreadPool(QThreadPool* threadPool,
            std::function<ResultOrError<TResult2, TError2>(OutcomeType)> f);

        void handleOnEventLoop(QObject* receiver, std::function<void(OutcomeType)> f);

        // TODO: adding listeners

    private:
        using StorageType = NewFutureStorage<TResult, TError>;
        using StoragePtr = QSharedPointer<NewFutureStorage<TResult, TError>>;
        using ContinuationPtr = QSharedPointer<Continuation<TResult, TError>>;

        NewFuture(StoragePtr storage);

        static NewFuture<TResult, TError> createForRunner(
            QSharedPointer<Runner> runner,
            std::function<OutcomeType()> f);

        template<class, class> friend class NewFuture;
        friend class NewPromise<TResult, TError>;
        friend class NewAsync;
        friend class NewConcurrent;

        StoragePtr _storage;
    };

    template<class TResult, class TError>
    NewFuture<TResult, TError>::NewFuture(const NewFutureResult<TResult>& result)
        : _storage(StorageType::createWithResult(result._result))
    {
        //
    }

    template<class TResult, class TError>
    NewFuture<TResult, TError>::NewFuture(const NewFutureError<TError>& error)
        : _storage(StorageType::createWithError(error._error))
    {
        //
    }

    template<class TResult, class TError>
    template<class TResult2, class TError2>
    NewFuture<TResult2, TError2>
    NewFuture<TResult, TError>::thenOnThreadPool(
        QThreadPool* threadPool,
        std::function<ResultOrError<TResult2, TError2> (OutcomeType)> f)
    {
        auto runner = QSharedPointer<ThreadPoolRunner>::create(threadPool);

        auto storage = QSharedPointer<NewFutureStorage<TResult2, TError2>>::create();

        auto wrapper =
            [f, storage, runner](OutcomeType input)
            {
                auto resultOrError = f(input);

                // store result, notify listeners, and run continuation
                storage->storeAndContinueFrom(resultOrError, runner);
            };

        auto continuation = ContinuationPtr::create(runner, wrapper);

        _storage->setContinuation(continuation);

        return NewFuture<TResult2, TError2>(storage);
    }

    template<class TResult, class TError>
    void NewFuture<TResult, TError>::handleOnEventLoop(
        QObject* receiver,
        std::function<void (OutcomeType)> f)
    {
        auto runner = QSharedPointer<EventLoopRunner>::create(receiver);

        auto continuation = ContinuationPtr::create(runner, f);

        _storage->setContinuation(continuation);
    }

    template<class TResult, class TError>
    NewFuture<TResult, TError>::NewFuture(StoragePtr storage)
        : _storage(storage)
    {
        //
    }

    template<class TResult, class TError>
    NewFuture<TResult, TError> NewFuture<TResult, TError>::createForRunner(
        QSharedPointer<Runner> runner,
        std::function<OutcomeType ()> f)
    {
        auto storage = StoragePtr::create();

        auto wrapper =
            [f, storage, runner]()
        {
            auto outcome = f();

            // store result, notify listeners, and run continuation
            storage->storeAndContinueFrom(outcome, runner);
        };

        runner->run(wrapper);

        return NewFuture<TResult, TError>(storage);
    }

    // =================================================================== //

    template<class TOutcome>
    class NewSimpleFuture
    {
    public:
        using OutcomeType = TOutcome;

        static NewSimpleFuture<TOutcome> fromOutcome(TOutcome const& result);

        void handleOnEventLoop(QObject* receiver, std::function<void(OutcomeType)> f);

    private:
        using StorageType = NewFutureStorage<TOutcome, FailureType>;
        using StoragePtr = QSharedPointer<NewFutureStorage<TOutcome, FailureType>>;
        using ContinuationPtr = QSharedPointer<Continuation<TOutcome, FailureType>>;

        NewSimpleFuture(StoragePtr storage);

        static NewSimpleFuture<TOutcome> createForRunner(
            QSharedPointer<Runner> runner,
            std::function<OutcomeType()> f);

        friend class NewSimplePromise<TOutcome>;

        StoragePtr _storage;
    };

    template<class TOutcome>
    NewSimpleFuture<TOutcome> NewSimpleFuture<TOutcome>::fromOutcome(
        const TOutcome& result)
    {
        auto storage = StorageType::createWithResult(result);

        return NewSimpleFuture<TOutcome>(storage);
    }

    template<class TOutcome>
    inline void NewSimpleFuture<TOutcome>::handleOnEventLoop(
        QObject* receiver,
        std::function<void (OutcomeType)> f)
    {
        auto runner = QSharedPointer<EventLoopRunner>::create(receiver);

        auto wrapper =
            [f](ResultOrError<TOutcome, FailureType> outcome)
            {
                f(outcome.result());
            };

        auto continuation = ContinuationPtr::create(runner, wrapper);

        _storage->setContinuation(continuation);
    }

    template<class TOutcome>
    NewSimpleFuture<TOutcome>::NewSimpleFuture(StoragePtr storage)
        : _storage(storage)
    {
        //
    }

    template<class TOutcome>
    NewSimpleFuture<TOutcome> NewSimpleFuture<TOutcome>::createForRunner(
        QSharedPointer<Runner> runner,
        std::function<OutcomeType ()> f)
    {
        auto storage = StoragePtr::create();

        auto wrapper =
            [f, storage, runner]()
        {
            auto outcome = f();

            auto resultOrError =
                ResultOrError<TOutcome, FailureType>::fromResult(outcome);

            // store result, notify listeners, and run continuation
            storage->storeAndContinueFrom(resultOrError, runner);
        };

        runner->run(wrapper);

        return NewSimpleFuture<TOutcome>(storage);
    }
}
#endif
