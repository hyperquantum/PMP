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

#ifndef PMP_LAZYPROMISEDVALUE_H
#define PMP_LAZYPROMISEDVALUE_H

#include "nullable.h"
#include "promise.h"

#include <functional>

#include <QSharedPointer>

namespace PMP
{
    template <class ResultType, class ErrorType>
    class LazyPromisedValue
    {
    public:
        LazyPromisedValue(std::function<void ()> requester)
         : _requester(requester)
        {
            //
        }

        Future<ResultType, ErrorType> future()
        {
            if (_cached.hasValue())
                return Future<ResultType, ErrorType>::fromResultOrError(_cached.value());

            if (_promise.isNull())
            {
                _promise = QSharedPointer<Promise<ResultType, ErrorType>>::create();
                _requester();
            }

            return _promise->future();
        }

        void setResult(ResultType result)
        {
            _cached = ResultOrError<ResultType, ErrorType>::fromResult(result);

            if (_promise.isNull())
                return;

            _promise->setResult(result);
            _promise.clear();
        }

        void setError(ErrorType error)
        {
            _cached = ResultOrError<ResultType, ErrorType>::fromError(error);

            if (_promise.isNull())
                return;

            _promise->setError(error);
            _promise.clear();
        }

        void reset()
        {
            _cached = null;
            _promise.clear();
        }

    private:
        std::function<void ()> _requester;
        Nullable<ResultOrError<ResultType, ErrorType>> _cached;
        QSharedPointer<Promise<ResultType, ErrorType>> _promise;
    };
}
#endif
