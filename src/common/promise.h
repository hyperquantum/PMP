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

namespace PMP
{
    template<class ResultType, class ErrorType>
    class Promise
    {
    public:
        Promise()
         : _storage(QSharedPointer<FutureStorage<ResultType, ErrorType>>::create())
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

        void setResult(ResultType result)
        {
            _storage->setResult(result);
        }

        void setFailed(ErrorType error)
        {
            _storage->setFailed(error);
        }

    private:
        QSharedPointer<FutureStorage<ResultType, ErrorType>> _storage;
    };

    template<class ResultType>
    class Promise<ResultType, void>
    {
    public:
        Promise()
         : _storage(QSharedPointer<FutureStorage<ResultType, int>>::create())
        {
            //
        }

        Promise(Promise&&) = default;

        Promise(Promise const&) = delete;
        Promise& operator=(Promise const&) = delete;

        FutureResult<ResultType> futureResult()
        {
            return FutureResult<ResultType>(_storage);
        }

        void setResult(ResultType result)
        {
            _storage->setResult(result);
        }

    private:
        QSharedPointer<FutureStorage<ResultType, int>> _storage;
    };

    template<class ErrorType>
    class Promise<void, ErrorType>
    {
    public:
        Promise()
         : _storage(QSharedPointer<FutureStorage<int, ErrorType>>::create())
        {
            //
        }

        Promise(Promise&&) = default;

        Promise(Promise const&) = delete;
        Promise& operator=(Promise const&) = delete;

        FutureAction<ErrorType> futureAction()
        {
            return FutureAction<ErrorType>(_storage);
        }

        void setSuccessful()
        {
            _storage->setResult(0);
        }

        void setFailed(ErrorType error)
        {
            _storage->setFailed(error);
        }

    private:
        QSharedPointer<FutureStorage<int, ErrorType>> _storage;
    };
}
#endif
