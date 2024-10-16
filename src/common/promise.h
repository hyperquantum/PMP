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

#ifndef PMP_COMMON_PROMISE_H
#define PMP_COMMON_PROMISE_H

#include "future.h"

namespace PMP
{
    template<class TResult, class TError>
    class Promise
    {
    public:
        using OutcomeType = ResultOrError<TResult, TError>;
        using FutureType = Future<TResult, TError>;

        FutureType future() const;

        void setOutcome(OutcomeType const& outcome);
        void setResult(TResult const& result);
        void setError(TError const& error);

    private:
        Promise();

        friend class Async;

        QSharedPointer<FutureStorage<TResult, TError>> _storage;
    };

    template<class TResult, class TError>
    Future<TResult, TError> Promise<TResult, TError>::future() const
    {
        return Future<TResult, TError>(_storage);
    }

    template<class TResult, class TError>
    void Promise<TResult, TError>::setOutcome(const OutcomeType& outcome)
    {
        _storage->storeAndContinueFrom(outcome, nullptr);
    }

    template<class TResult, class TError>
    inline void Promise<TResult, TError>::setResult(const TResult& result)
    {
        setOutcome(OutcomeType::fromResult(result));
    }

    template<class TResult, class TError>
    inline void Promise<TResult, TError>::setError(const TError& error)
    {
        setOutcome(OutcomeType::fromError(error));
    }

    template<class TResult, class TError>
    Promise<TResult, TError>::Promise()
        : _storage(QSharedPointer<FutureStorage<TResult, TError>>::create())
    {
        //
    }

    // =================================================================== //

    template<class TOutcome>
    class SimplePromise
    {
    public:
        using OutcomeType = TOutcome;
        using FutureType = SimpleFuture<TOutcome>;

        FutureType future() const;

        void setOutcome(OutcomeType const& outcome);

    private:
        SimplePromise();

        friend class Async;

        QSharedPointer<FutureStorage<TOutcome, FailureType>> _storage;
    };

    template<class TOutcome>
    SimpleFuture<TOutcome> SimplePromise<TOutcome>::future() const
    {
        return SimpleFuture<TOutcome>(_storage);
    }

    template<class TOutcome>
    void SimplePromise<TOutcome>::setOutcome(const OutcomeType& outcome)
    {
        auto resultOrError = ResultOrError<TOutcome, FailureType>::fromResult(outcome);

        _storage->storeAndContinueFrom(resultOrError, nullptr);
    }

    template<class TOutcome>
    SimplePromise<TOutcome>::SimplePromise()
        : _storage(QSharedPointer<FutureStorage<TOutcome, FailureType>>::create())
    {
        //
    }
}
#endif
