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
        NewFuture<TResult, TError> future() const;

        void setOutcome(ResultOrError<TResult, TError> const& outcome);
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
    void NewPromise<TResult, TError>::setOutcome(
        ResultOrError<TResult, TError> const& outcome)
    {
        _storage->storeAndContinueFrom(outcome, nullptr);
    }

    template<class TResult, class TError>
    inline void NewPromise<TResult, TError>::setResult(const TResult& result)
    {
        setOutcome(ResultOrError<TResult, TError>::fromResult(result));
    }

    template<class TResult, class TError>
    inline void NewPromise<TResult, TError>::setError(const TError& error)
    {
        setOutcome(ResultOrError<TResult, TError>::fromError(error));
    }

    template<class TResult, class TError>
    NewPromise<TResult, TError>::NewPromise()
        : _storage(QSharedPointer<NewFutureStorage<TResult, TError>>::create())
    {
        //
    }
}
#endif
