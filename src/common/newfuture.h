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
        NewFutureStorage() {}

        static QSharedPointer<NewFutureStorage<TResult, TError>> create();

        ResultOrError<TResult, TError> getOutcomeInternal() const;

        void setContinuation(QSharedPointer<Continuation<TResult, TError>> continuation);

        void storeAndContinueFrom(ResultOrError<TResult, TError> const& outcome,
                                  QSharedPointer<Runner> runner);

        friend class QSharedPointer<NewFutureStorage<TResult, TError>>;
        friend class NewPromise<TResult, TError>;
        template<class, class> friend class NewFuture;
        friend class NewConcurrent;

        QMutex _mutex;
        QSharedPointer<Continuation<TResult, TError>> _continuation;
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
    void NewFutureStorage<TResult, TError>::setContinuation(
        QSharedPointer<Continuation<TResult, TError>> continuation)
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

    template<class TResult, class TError>
    class NewFuture
    {
    public:
        template<class TResult2, class TError2>
        NewFuture<TResult2, TError2> thenOnThreadPool(QThreadPool* threadPool,
            std::function<ResultOrError<TResult2, TError2>(ResultOrError<TResult, TError>)> f);

        void handleOnEventLoop(QObject* receiver,
                               std::function<void(ResultOrError<TResult, TError>)> f);

        // TODO: adding listeners

    private:
        NewFuture(QSharedPointer<NewFutureStorage<TResult, TError>> storage);

        static NewFuture<TResult, TError> createForRunner(
            QSharedPointer<Runner> runner,
            std::function<ResultOrError<TResult, TError>()> f);

        template<class, class> friend class NewFuture;
        friend class NewPromise<TResult, TError>;
        friend class NewAsync;
        friend class NewConcurrent;

        QSharedPointer<NewFutureStorage<TResult, TError>> _storage;
    };

    template<class TResult, class TError>
    template<class TResult2, class TError2>
    NewFuture<TResult2, TError2>
        NewFuture<TResult, TError>::thenOnThreadPool(QThreadPool* threadPool,
            std::function<ResultOrError<TResult2, TError2> (ResultOrError<TResult, TError>)> f)
    {
        auto runner = QSharedPointer<ThreadPoolRunner>::create(threadPool);

        auto storage = QSharedPointer<NewFutureStorage<TResult2, TError2>>::create();

        auto wrapper =
            [f, storage, runner](ResultOrError<TResult, TError> input)
            {
                auto resultOrError = f(input);

                // store result, notify listeners, and run continuation
                storage->storeAndContinueFrom(resultOrError, runner);
            };

        auto continuation =
            QSharedPointer<Continuation<TResult, TError>>::create(runner, wrapper);

        _storage->setContinuation(continuation);

        return NewFuture<TResult2, TError2>(storage);
    }

    template<class TResult, class TError>
    void NewFuture<TResult, TError>::handleOnEventLoop(QObject* receiver,
                                std::function<void (ResultOrError<TResult, TError>)> f)
    {
        auto runner = QSharedPointer<EventLoopRunner>::create(receiver);

        auto continuation =
            QSharedPointer<Continuation<TResult, TError>>::create(runner, f);

        _storage->setContinuation(continuation);
    }

    template<class TResult, class TError>
    NewFuture<TResult, TError>::NewFuture(
                                QSharedPointer<NewFutureStorage<TResult, TError>> storage)
        : _storage(storage)
    {
        //
    }

    template<class TResult, class TError>
    NewFuture<TResult, TError> NewFuture<TResult, TError>::createForRunner(
        QSharedPointer<Runner> runner, std::function<ResultOrError<TResult, TError> ()> f)
    {
        auto storage = QSharedPointer<NewFutureStorage<TResult, TError>>::create();

        auto wrapper =
            [f, storage, runner]()
        {
            auto resultOrError = f();

            // store result, notify listeners, and run continuation
            storage->storeAndContinueFrom(resultOrError, runner);
        };

        runner->run(wrapper);

        return NewFuture<TResult, TError>(storage);
    }
}
#endif
