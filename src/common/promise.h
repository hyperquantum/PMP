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

#ifndef PMP_PROMISE_H
#define PMP_PROMISE_H

#include "future.h"
#include "resultorerror.h"

namespace PMP
{
    template<class ResultType, class ErrorType>
    class Promise
    {
    public:
        Promise()
         : _storage(FutureStorage<ResultType, ErrorType>::create())
        {
            //
        }

        Promise(Promise&&) = default;

        Promise(Promise const&) = delete;
        Promise& operator=(Promise const&) = delete;

        Future<ResultType, ErrorType> future() const
        {
            return Future<ResultType, ErrorType>(_storage);
        }

        void setOutcome(ResultOrError<ResultType, ErrorType> const& r)
        {
            _storage->setOutcome(r);
        }

        void setResult(ResultType result)
        {
            _storage->setResult(result);
        }

        void setError(ErrorType error)
        {
            _storage->setError(error);
        }

    private:
        QSharedPointer<FutureStorage<ResultType, ErrorType>> _storage;
    };

    template<class T>
    class SimplePromise
    {
    public:
        SimplePromise()
         : _storage(FutureStorage<T, FailureType>::create())
        {
            //
        }

        SimplePromise(SimplePromise&&) = default;

        SimplePromise(SimplePromise const&) = delete;
        SimplePromise& operator=(SimplePromise const&) = delete;

        SimpleFuture<T> future() const
        {
            return SimpleFuture<T>(_storage);
        }

        void setResult(T result)
        {
            _storage->setResult(result);
        }

        void connectToResultFrom(SimpleFuture<T> const& future)
        {
            future._storage->addResultListener(
                [storage = _storage](T result) { storage->setResult(result); }
            );
        }

    private:
        QSharedPointer<FutureStorage<T, FailureType>> _storage;
    };
}
#endif
