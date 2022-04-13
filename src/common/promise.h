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

        Future<ResultType, ErrorType> future()
        {
            return Future<ResultType, ErrorType>(_storage);
        }

        void setResult(ResultOrError<ResultType, ErrorType> const& r)
        {
            if (r.succeeded())
                setResult(r.result());
            else
                setError(r.error());
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

    template<class ResultType>
    class Promise<ResultType, void>
    {
    public:
        Promise()
         : _storage(FutureStorage<ResultType, FailureType>::create())
        {
            //
        }

        Promise(Promise&&) = default;

        Promise(Promise const&) = delete;
        Promise& operator=(Promise const&) = delete;

        Future<ResultType, void> future()
        {
            return Future<ResultType, void>(_storage);
        }

        void setResult(ResultOrError<ResultType, void> const& r)
        {
            if (r.succeeded())
                setResult(r.result());
            else
                setError();
        }

        void setResult(ResultType result)
        {
            _storage->setResult(result);
        }

        void setError()
        {
            _storage->setError(failure);
        }

    private:
        QSharedPointer<FutureStorage<ResultType, FailureType>> _storage;
    };

    template<class ErrorType>
    class Promise<void, ErrorType>
    {
    public:
        Promise()
         : _storage(FutureStorage<SuccessType, ErrorType>::create())
        {
            //
        }

        Promise(Promise&&) = default;

        Promise(Promise const&) = delete;
        Promise& operator=(Promise const&) = delete;

        Future<void, ErrorType> future()
        {
            return Future<void, ErrorType>(_storage);
        }

        void setResult(ResultOrError<void, ErrorType> const& r)
        {
            if (r.succeeded())
                setSuccessful();
            else
                setError(r.error());
        }

        void setSuccessful()
        {
            _storage->setResult(success);
        }

        void setError(ErrorType error)
        {
            _storage->setError(error);
        }

    private:
        QSharedPointer<FutureStorage<SuccessType, ErrorType>> _storage;
    };

    template<>
    class Promise<void, void>
    {
    public:
        Promise()
         : _storage(FutureStorage<SuccessType, FailureType>::create())
        {
            //
        }

        Promise(Promise&&) = default;

        Promise(Promise const&) = delete;
        Promise& operator=(Promise const&) = delete;

        Future<void, void> future()
        {
            return Future<void, void>(_storage);
        }

        void setResult(ResultOrError<void, void> const& r)
        {
            if (r.succeeded())
                setSuccessful();
            else
                setFailed();
        }

        void setSuccessful()
        {
            _storage->setResult(success);
        }

        void setFailed()
        {
            _storage->setError(failure);
        }

    private:
        QSharedPointer<FutureStorage<SuccessType, FailureType>> _storage;
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

        SimpleFuture<T> future()
        {
            return SimpleFuture<T>(_storage);
        }

        void setResult(T result)
        {
            _storage->setResult(result);
        }

    private:
        QSharedPointer<FutureStorage<T, FailureType>> _storage;
    };

    class VoidPromise
    {
    public:
        VoidPromise()
         : _storage(FutureStorage<SuccessType, FailureType>::create())
        {
            //
        }

        VoidPromise(VoidPromise&&) = default;

        VoidPromise(VoidPromise const&) = delete;
        VoidPromise& operator=(VoidPromise const&) = delete;

        VoidFuture future()
        {
            return VoidFuture(_storage);
        }

        void setFinished()
        {
            _storage->setResult(success);
        }

    private:
        QSharedPointer<FutureStorage<SuccessType, FailureType>> _storage;
    };
}
#endif
