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

#include <list>

namespace PMP
{
    template<class TResult, class TError> class Promise;
    template<class TResult, class TError> class Future;
    template<class TOutcome> class SimplePromise;
    template<class TOutcome> class SimpleFuture;
    template<class TResult, class TError> class FutureStorage;
    template<class TResult, class TError> class Continuation;

    // =================================================================== //

    template<class TResult, class TError>
    class Continuation
    {
    public:
        Continuation(QSharedPointer<Runner> runner,
                     std::function<void (QSharedPointer<Runner> actualRunner,
                                ResultOrError<TResult, TError> previousOutcome)> work);

        Continuation(Continuation const&) = delete;

        void continueFrom(QSharedPointer<Runner> previousRunner,
                          ResultOrError<TResult, TError> const& previousOutcome);

        Continuation& operator=(Continuation const&) = delete;

    private:
        friend class QSharedPointer<Continuation<TResult, TError>>;

        QSharedPointer<Runner> const _runner;
        std::function<void (QSharedPointer<Runner> actualRunner,
                            ResultOrError<TResult, TError> previousOutcome)> const _work;
    };

    template<class TResult, class TError>
    Continuation<TResult, TError>::Continuation(QSharedPointer<Runner> runner,
        std::function<void (QSharedPointer<Runner>, ResultOrError<TResult, TError>)> work)
        : _runner(runner), _work(work)
    {
        //
    }

    template<class TResult, class TError>
    void Continuation<TResult, TError>::continueFrom(
                                    QSharedPointer<Runner> previousRunner,
                                    ResultOrError<TResult, TError> const& previousOutcome)
    {
        if (previousRunner && _runner->canContinueInThreadFrom(previousRunner.data()))
        {
            _work(previousRunner, previousOutcome);
            return;
        }

        auto wrapper =
            [work = _work, actualRunner = _runner, previousOutcome]()
            {
                work(actualRunner, previousOutcome);
            };

        _runner->run(wrapper);
    }

    // =================================================================== //

    template<class TResult, class TError>
    class FutureStorage
    {
    public:
        using StoragePtr = QSharedPointer<FutureStorage<TResult, TError>>;
        using OutcomeType = ResultOrError<TResult, TError>;

        FutureStorage(FutureStorage const&) = delete;
        FutureStorage& operator=(FutureStorage const&) = delete;

    private:
        using ContinuationPtr = QSharedPointer<Continuation<TResult, TError>>;

        FutureStorage() {}

        static StoragePtr create();
        static StoragePtr createWithResult(TResult const& result);
        static StoragePtr createWithError(TError const& error);
        static StoragePtr createWithOutcome(OutcomeType const& outcome);

        static ContinuationPtr createContinuationThatStoresResultAt(StoragePtr storage);

        Nullable<ResultOrError<TResult, TError>> getOutcomeIfFinished();

        ResultOrError<TResult, TError> getOutcomeInternal() const;

        void addContinuation(ContinuationPtr continuation);

        void storeAndContinueFrom(ResultOrError<TResult, TError> const& outcome,
                                  QSharedPointer<Runner> runner);

        friend class QSharedPointer<FutureStorage<TResult, TError>>;
        friend class Promise<TResult, TError>;
        friend class SimplePromise<TResult>;
        template<class, class> friend class Future;
        friend class SimpleFuture<TResult>;
        friend class Concurrent;

        QMutex _mutex;
        std::list<ContinuationPtr> _continuations;
        Nullable<TResult> _result;
        Nullable<TError> _error;
        bool _finished { false };
    };

    template<class TResult, class TError>
    QSharedPointer<FutureStorage<TResult, TError>>
        FutureStorage<TResult, TError>::create()
    {
        return QSharedPointer<FutureStorage<TResult, TError>>::create();
    }

    template<class TResult, class TError>
    QSharedPointer<FutureStorage<TResult, TError>>
        FutureStorage<TResult, TError>::createWithResult(const TResult& result)
    {
        auto storage = StoragePtr::create();

        storage->_finished = true;
        storage->_result = result;

        return storage;
    }

    template<class TResult, class TError>
    QSharedPointer<FutureStorage<TResult, TError>>
        FutureStorage<TResult, TError>::createWithError(TError const& error)
    {
        auto storage = StoragePtr::create();

        storage->_finished = true;
        storage->_error = error;

        return storage;
    }

    template<class TResult, class TError>
    QSharedPointer<FutureStorage<TResult, TError>>
        FutureStorage<TResult, TError>::createWithOutcome(OutcomeType const& outcome)
    {
        auto storage = StoragePtr::create();

        storage->_finished = true;

        if (outcome.succeeded())
            storage->_result = outcome.result();
        else
            storage->_error = outcome.error();

        return storage;
    }

    template<class TResult, class TError>
    QSharedPointer<Continuation<TResult, TError>>
        FutureStorage<TResult, TError>::createContinuationThatStoresResultAt(
            StoragePtr storage)
    {
        auto wrapper =
            [storage](QSharedPointer<Runner> actualRunner, OutcomeType previousOutcome)
            {
                storage->storeAndContinueFrom(previousOutcome, actualRunner);
            };

        auto runner = QSharedPointer<AnyThreadContinuationRunner>::create();
        auto continuation = ContinuationPtr::create(runner, wrapper);

        return continuation;
    }

    template<class TResult, class TError>
    Nullable<ResultOrError<TResult, TError>>
        FutureStorage<TResult, TError>::getOutcomeIfFinished()
    {
        QMutexLocker lock(&_mutex);

        if (!_finished)
            return null;

        return getOutcomeInternal();
    }

    template<class TResult, class TError>
    ResultOrError<TResult, TError>
        FutureStorage<TResult, TError>::getOutcomeInternal() const
    {
        Q_ASSERT_X(_finished, "NewFutureStorage::getResultInternal()",
                   "attempt to get result of unfinished future");

        if (_result.hasValue())
            return _result.value();
        else
            return _error.value();
    }

    template<class TResult, class TError>
    void FutureStorage<TResult, TError>::addContinuation(ContinuationPtr continuation)
    {
        QMutexLocker lock(&_mutex);

        Q_ASSERT_X(continuation, "NewFutureStorage::setContinuation()",
                   "continuation is nullptr");

        // when already finished, continue immediately, do not add the continuation
        if (_finished)
        {
            auto outcome = getOutcomeInternal();
            lock.unlock(); /* unlock before continuing */
            continuation->continueFrom(nullptr, outcome);
            return;
        }

        _continuations.push_back(continuation);
    }

    template<class TResult, class TError>
    void FutureStorage<TResult, TError>::storeAndContinueFrom(
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

        std::list<ContinuationPtr> continuations;
        swap(_continuations, continuations);

        lock.unlock(); /* unlock before continuing */

        for (auto& continuation : continuations)
        {
            if (continuation)
            {
                continuation->continueFrom(runner, outcome);
            }
        }
    }

    // =================================================================== //

    template<class TResult>
    class FutureResult
    {
    public:
        FutureResult(TResult const& result) : _result(result) {}
        FutureResult(TResult&& result) : _result(result) {}

    private:
        template<class T1, class T2> friend class Future;
        friend class SimpleFuture<TResult>;

        TResult _result;
    };

    template<class TError>
    class FutureError
    {
    public:
        FutureError(TError const& error) : _error(error) {}
        FutureError(TError&& error) : _error(error) {}

    private:
        template<class T1, class T2> friend class Future;

        TError _error;
    };

    // =================================================================== //

    template<class TResult, class TError>
    class Future
    {
    public:
        using OutcomeType = ResultOrError<TResult, TError>;

        Future(FutureResult<TResult> const& result);
        Future(FutureError<TError> const& error);

        static Future<TResult, TError> fromOutcome(OutcomeType outcome);

        Nullable<OutcomeType> outcomeIfFinished();

        template<class TResult2, class TError2>
        Future<TResult2, TError2> thenOnThreadPool(ThreadPoolSpecifier threadPool,
            std::function<ResultOrError<TResult2, TError2>(OutcomeType)> f);

        template<class TResult2, class TError2>
        Future<TResult2, TError2> thenOnEventLoop(QObject* receiver,
            std::function<ResultOrError<TResult2, TError2>(OutcomeType)> f);

        /*
        template<class TResult2, class TError2>
        Future<TResult2, TError2> thenOnEventLoopIndirect(QObject* receiver,
            std::function<Future<TResult2, TError2>(OutcomeType)> f);
        */

        template<class TResult2, class TError2>
        Future<TResult2, TError2> thenOnAnyThread(
            std::function<ResultOrError<TResult2, TError2>(OutcomeType)> f);

        template<class TResult2, class TError2>
        Future<TResult2, TError2> thenOnAnyThreadIndirect(
            std::function<Future<TResult2, TError2>(OutcomeType)> f);

        void handleOnEventLoop(QObject* receiver, std::function<void(OutcomeType)> f);

        template<class TOutcome2>
        SimpleFuture<TOutcome2> convertToSimpleFuture(
            std::function<TOutcome2 (TResult const& result)> resultConverter,
            std::function<TOutcome2 (TError const& error)> errorConverter);

    private:
        using StorageType = FutureStorage<TResult, TError>;
        using StoragePtr = QSharedPointer<FutureStorage<TResult, TError>>;
        using ContinuationPtr = QSharedPointer<Continuation<TResult, TError>>;

        Future(StoragePtr storage);

        static Future<TResult, TError> createForRunnerDirect(
            QSharedPointer<Runner> runner,
            std::function<OutcomeType()> f);

        static Future<TResult, TError> createForRunnerIndirect(
            QSharedPointer<Runner> runner,
            std::function<Future<TResult, TError>()> f);

        template<class TResult2, class TError2>
        Future<TResult2, TError2> setUpContinuationToRunner(
            QSharedPointer<Runner> runner,
            std::function<ResultOrError<TResult2, TError2>(OutcomeType)> f);

        template<class TResult2, class TError2>
        Future<TResult2, TError2> setUpContinuationToRunnerIndirect(
            QSharedPointer<Runner> runner,
            std::function<Future<TResult2, TError2>(OutcomeType)> f);

        template<class TOutcome2>
        SimpleFuture<TOutcome2> setUpContinuationToRunnerForSimpleFuture(
            QSharedPointer<Runner> runner,
            std::function<TOutcome2(OutcomeType)> f);

        template<class, class> friend class Future;
        friend class Promise<TResult, TError>;
        friend class Async;
        friend class Concurrent;

        StoragePtr _storage;
    };

    template<class TResult, class TError>
    Future<TResult, TError>::Future(const FutureResult<TResult>& result)
        : _storage(StorageType::createWithResult(result._result))
    {
        //
    }

    template<class TResult, class TError>
    Future<TResult, TError>::Future(const FutureError<TError>& error)
        : _storage(StorageType::createWithError(error._error))
    {
        //
    }

    template<class TResult, class TError>
    inline Future<TResult, TError> Future<TResult, TError>::fromOutcome(
                                                                    OutcomeType outcome)
    {
        return Future(StorageType::createWithOutcome(outcome));
    }

    template<class TResult, class TError>
    Nullable<ResultOrError<TResult, TError>>
        Future<TResult, TError>::outcomeIfFinished()
    {
        return _storage->getOutcomeIfFinished();
    }

    template<class TResult, class TError>
    template<class TResult2, class TError2>
    Future<TResult2, TError2>
    Future<TResult, TError>::thenOnThreadPool(
        ThreadPoolSpecifier threadPool,
        std::function<ResultOrError<TResult2, TError2> (OutcomeType)> f)
    {
        auto runner = QSharedPointer<ThreadPoolRunner>::create(threadPool);

        return setUpContinuationToRunner<TResult2, TError2>(runner, f);
    }

    template<class TResult, class TError>
    template<class TResult2, class TError2>
    Future<TResult2, TError2>
        Future<TResult, TError>::thenOnEventLoop(
            QObject* receiver,
            std::function<ResultOrError<TResult2, TError2> (OutcomeType)> f)
    {
        auto runner = QSharedPointer<EventLoopRunner>::create(receiver);

        return setUpContinuationToRunner<TResult2, TError2>(runner, f);
    }

    /*
    template<class TResult, class TError>
    template<class TResult2, class TError2>
    Future<TResult2, TError2>
        Future<TResult, TError>::thenOnEventLoopIndirect(
            QObject* receiver,
            std::function<NewFuture<TResult2, TError2> (OutcomeType)> f)
    {
        auto runner = QSharedPointer<EventLoopRunner>::create(receiver);

        return setUpContinuationToRunnerIndirect<TResult2, TError2>(runner, f);
    }
    */

    template<class TResult, class TError>
    template<class TResult2, class TError2>
    Future<TResult2, TError2>
        Future<TResult, TError>::thenOnAnyThread(
            std::function<ResultOrError<TResult2, TError2> (OutcomeType)> f)
    {
        auto runner = QSharedPointer<AnyThreadContinuationRunner>::create();

        return setUpContinuationToRunner(runner, f);
    }

    template<class TResult, class TError>
    template<class TResult2, class TError2>
    Future<TResult2, TError2>
        Future<TResult, TError>::thenOnAnyThreadIndirect(
            std::function<Future<TResult2, TError2> (OutcomeType)> f)
    {
        auto runner = QSharedPointer<AnyThreadContinuationRunner>::create();

        return setUpContinuationToRunnerIndirect(runner, f);
    }

    template<class TResult, class TError>
    void Future<TResult, TError>::handleOnEventLoop(
        QObject* receiver,
        std::function<void (OutcomeType)> f)
    {
        auto runner = QSharedPointer<EventLoopRunner>::create(receiver);

        auto wrapper =
            [f](QSharedPointer<Runner> actualRunner, OutcomeType previousOutcome)
            {
                Q_UNUSED(actualRunner)

                f(previousOutcome);
            };

        auto continuation = ContinuationPtr::create(runner, wrapper);

        _storage->addContinuation(continuation);
    }

    template<class TResult, class TError>
    template<class TOutcome2>
    SimpleFuture<TOutcome2> Future<TResult, TError>::convertToSimpleFuture(
        std::function<TOutcome2 (const TResult&)> resultConverter,
        std::function<TOutcome2 (const TError&)> errorConverter)
    {
        auto runner = QSharedPointer<AnyThreadContinuationRunner>::create();

        auto conversion =
            [resultConverter, errorConverter](OutcomeType input) -> TOutcome2
            {
                if (input.succeeded())
                    return resultConverter(input.result());
                else
                    return errorConverter(input.error());
            };

        return setUpContinuationToRunnerForSimpleFuture<TOutcome2>(runner, conversion);
    }

    template<class TResult, class TError>
    Future<TResult, TError>::Future(StoragePtr storage)
        : _storage(storage)
    {
        //
    }

    template<class TResult, class TError>
    Future<TResult, TError> Future<TResult, TError>::createForRunnerDirect(
        QSharedPointer<Runner> runner,
        std::function<OutcomeType ()> f)
    {
        auto storage = StoragePtr::create();

        auto wrapper =
            [f, storage, runner]()
            {
                auto outcome = f();

                // store result and run continuation
                storage->storeAndContinueFrom(outcome, runner);
            };

        runner->run(wrapper);

        return Future<TResult, TError>(storage);
    }

    template<class TResult, class TError>
    Future<TResult, TError> Future<TResult, TError>::createForRunnerIndirect(
        QSharedPointer<Runner> runner,
        std::function<Future<TResult, TError> ()> f)
    {
        auto storage = StoragePtr::create();
        auto continuation = StorageType::createContinuationThatStoresResultAt(storage);

        auto wrapper =
            [f, continuation]()
            {
                auto future = f();

                future._storage->addContinuation(continuation);
            };

        runner->run(wrapper);

        return Future<TResult, TError>(storage);
    }

    template<class TResult, class TError>
    template<class TResult2, class TError2>
    Future<TResult2, TError2>
        Future<TResult, TError>::setUpContinuationToRunner(
            QSharedPointer<Runner> runner,
            std::function<ResultOrError<TResult2, TError2> (OutcomeType)> f)
    {
        auto storage = QSharedPointer<FutureStorage<TResult2, TError2>>::create();

        auto wrapper =
            [f, storage](QSharedPointer<Runner> actualRunner, OutcomeType previousOutcome)
            {
                auto resultOrError = f(previousOutcome);

                // store result and run continuation
                storage->storeAndContinueFrom(resultOrError, actualRunner);
            };

        auto continuation = ContinuationPtr::create(runner, wrapper);
        _storage->addContinuation(continuation);

        return Future<TResult2, TError2>(storage);
    }

    template<class TResult, class TError>
    template<class TResult2, class TError2>
    Future<TResult2, TError2>
        Future<TResult, TError>::setUpContinuationToRunnerIndirect(
            QSharedPointer<Runner> runner,
            std::function<Future<TResult2, TError2> (OutcomeType)> f)
    {
        using Storage2Type = FutureStorage<TResult2, TError2>;
        auto secondStorage = QSharedPointer<Storage2Type>::create();
        auto secondContinuation =
            Storage2Type::createContinuationThatStoresResultAt(secondStorage);

        auto wrapper =
            [f, secondContinuation](QSharedPointer<Runner> actualRunner,
                                    OutcomeType previousOutcome)
            {
                Q_UNUSED(actualRunner)

                auto otherFuture = f(previousOutcome);

                otherFuture._storage->addContinuation(secondContinuation);
            };

        auto continuation = ContinuationPtr::create(runner, wrapper);
        _storage->addContinuation(continuation);

        return Future<TResult2, TError2>(secondStorage);
    }

    template<class TResult, class TError>
    template<class TOutcome2>
    SimpleFuture<TOutcome2>
        Future<TResult, TError>::setUpContinuationToRunnerForSimpleFuture(
            QSharedPointer<Runner> runner,
            std::function<TOutcome2 (OutcomeType)> f)
    {
        auto storage = QSharedPointer<FutureStorage<TOutcome2, FailureType>>::create();

        auto wrapper =
            [f, storage](QSharedPointer<Runner> actualRunner, OutcomeType previousOutcome)
            {
                auto outcome = f(previousOutcome);

                auto resultOrError =
                    ResultOrError<TOutcome2, FailureType>::fromResult(outcome);

                // store result and run continuation
                storage->storeAndContinueFrom(resultOrError, actualRunner);
            };

        auto continuation = ContinuationPtr::create(runner, wrapper);
        _storage->addContinuation(continuation);

        return SimpleFuture<TOutcome2>(storage);
    }

    // =================================================================== //

    template<class TOutcome>
    class SimpleFuture
    {
    public:
        using OutcomeType = TOutcome;

        SimpleFuture(FutureResult<TOutcome> const& outcome);

        static SimpleFuture<TOutcome> fromOutcome(TOutcome const& outcome);

        void handleOnEventLoop(QObject* receiver, std::function<void(OutcomeType)> f);

    private:
        using StorageType = FutureStorage<TOutcome, FailureType>;
        using StoragePtr = QSharedPointer<FutureStorage<TOutcome, FailureType>>;
        using ContinuationPtr = QSharedPointer<Continuation<TOutcome, FailureType>>;

        SimpleFuture(StoragePtr storage);

        static SimpleFuture<TOutcome> createForRunnerDirect(
            QSharedPointer<Runner> runner,
            std::function<OutcomeType()> f);

        static SimpleFuture<TOutcome> createForRunnerIndirect(
            QSharedPointer<Runner> runner,
            std::function<SimpleFuture<TOutcome>()> f);

        template<class T1, class T2> friend class Future;
        friend class SimplePromise<TOutcome>;
        friend class Async;

        StoragePtr _storage;
    };

    template<class TOutcome>
    SimpleFuture<TOutcome>::SimpleFuture(const FutureResult<TOutcome>& outcome)
        : _storage(StorageType::createWithResult(outcome._result))
    {
        //
    }

    template<class TOutcome>
    SimpleFuture<TOutcome> SimpleFuture<TOutcome>::fromOutcome(
        const TOutcome& outcome)
    {
        auto storage = StorageType::createWithResult(outcome);

        return SimpleFuture(storage);
    }

    template<class TOutcome>
    void SimpleFuture<TOutcome>::handleOnEventLoop(
        QObject* receiver,
        std::function<void (OutcomeType)> f)
    {
        auto runner = QSharedPointer<EventLoopRunner>::create(receiver);

        auto wrapper =
            [f](QSharedPointer<Runner> actualRunner,
                ResultOrError<TOutcome, FailureType> previousOutcome)
            {
                Q_UNUSED(actualRunner)

                f(previousOutcome.result());
            };

        auto continuation = ContinuationPtr::create(runner, wrapper);
        _storage->addContinuation(continuation);
    }

    template<class TOutcome>
    SimpleFuture<TOutcome>::SimpleFuture(StoragePtr storage)
        : _storage(storage)
    {
        //
    }

    template<class TOutcome>
    SimpleFuture<TOutcome> SimpleFuture<TOutcome>::createForRunnerDirect(
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

            // store result and run continuation
            storage->storeAndContinueFrom(resultOrError, runner);
        };

        runner->run(wrapper);

        return SimpleFuture<TOutcome>(storage);
    }

    template<class TOutcome>
    SimpleFuture<TOutcome> SimpleFuture<TOutcome>::createForRunnerIndirect(
        QSharedPointer<Runner> runner,
        std::function<SimpleFuture<TOutcome> ()> f)
    {
        auto storage = StoragePtr::create();
        auto continuation = StorageType::createContinuationThatStoresResultAt(storage);

        auto wrapper =
            [f, runner, continuation]()
            {
                auto future = f();

                future._storage->addContinuation(continuation);
            };

        runner->run(wrapper);

        return SimpleFuture<TOutcome>(storage);
    }
}
#endif
