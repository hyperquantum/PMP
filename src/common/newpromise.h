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

#ifndef PMP_COMMON_NEWPROMISE_H
#define PMP_COMMON_NEWPROMISE_H

#include "newfuture.h"

namespace PMP
{
    template<class TResult, class TError>
    class NewPromise
    {
    public:
        using OutcomeType = ResultOrError<TResult, TError>;
        using FutureType = NewFuture<TResult, TError>;

        FutureType future() const;

        void setOutcome(OutcomeType const& outcome);
        void setResult(TResult const& result);
        void setError(TError const& error);

    private:
        NewPromise();

        friend class NewAsync;

        QSharedPointer<NewFutureStorage<TResult, TError>> _storage;
    };

    template<class TResult, class TError>
    NewFuture<TResult, TError> NewPromise<TResult, TError>::future() const
    {
        return NewFuture<TResult, TError>(_storage);
    }

    template<class TResult, class TError>
    void NewPromise<TResult, TError>::setOutcome(const OutcomeType& outcome)
    {
        _storage->storeAndContinueFrom(outcome, nullptr);
    }

    template<class TResult, class TError>
    inline void NewPromise<TResult, TError>::setResult(const TResult& result)
    {
        setOutcome(OutcomeType::fromResult(result));
    }

    template<class TResult, class TError>
    inline void NewPromise<TResult, TError>::setError(const TError& error)
    {
        setOutcome(OutcomeType::fromError(error));
    }

    template<class TResult, class TError>
    NewPromise<TResult, TError>::NewPromise()
        : _storage(QSharedPointer<NewFutureStorage<TResult, TError>>::create())
    {
        //
    }

    // =================================================================== //

    template<class TOutcome>
    class NewSimplePromise
    {
    public:
        using OutcomeType = TOutcome;
        using FutureType = NewSimpleFuture<TOutcome>;

        FutureType future() const;

        void setOutcome(OutcomeType const& outcome);

    private:
        NewSimplePromise();

        friend class NewAsync;

        QSharedPointer<NewFutureStorage<TOutcome, FailureType>> _storage;
    };

    template<class TOutcome>
    NewSimpleFuture<TOutcome> NewSimplePromise<TOutcome>::future() const
    {
        return NewSimpleFuture<TOutcome>(_storage);
    }

    template<class TOutcome>
    void NewSimplePromise<TOutcome>::setOutcome(const OutcomeType& outcome)
    {
        auto resultOrError = ResultOrError<TOutcome, FailureType>::fromResult(outcome);

        _storage->storeAndContinueFrom(resultOrError, nullptr);
    }

    template<class TOutcome>
    NewSimplePromise<TOutcome>::NewSimplePromise()
        : _storage(QSharedPointer<NewFutureStorage<TOutcome, FailureType>>::create())
    {
        //
    }
}
#endif
