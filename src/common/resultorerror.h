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

#ifndef PMP_RESULTORERROR_H
#define PMP_RESULTORERROR_H

#include "nullable.h"

namespace PMP
{
    class SuccessType
    {
    public:
        constexpr SuccessType() {}

        constexpr bool operator==(SuccessType) { return true; }
        constexpr bool operator!=(SuccessType) { return false; }
    };

    constexpr SuccessType success = {};

    class FailureType
    {
    public:
        constexpr FailureType() {}

        constexpr bool operator==(FailureType) { return true; }
        constexpr bool operator!=(FailureType) { return false; }
    };

    constexpr FailureType failure = {};

    template<class ResultType, class ErrorType>
    class ResultOrError
    {
    public:
        static constexpr ResultOrError fromResult(ResultType result)
        {
            return ResultOrError(result, {});
        }

        static constexpr ResultOrError fromError(ErrorType error)
        {
            return ResultOrError({}, error);
        }

        constexpr ResultOrError(ResultType result) : _result(result) {}
        constexpr ResultOrError(ErrorType error) : _error(error) {}

        constexpr bool succeeded() const { return _result.hasValue(); }
        constexpr bool failed() const { return _result.isNull(); }

        ResultType result() const
        {
            Q_ASSERT_X(succeeded(), "ResultOrError::result()", "no result available");
            return _result.value();
        }

        ErrorType error() const
        {
            Q_ASSERT_X(failed(), "ResultOrError::error()", "no error available");
            return _error.value();
        }

    private:
        constexpr ResultOrError(Nullable<ResultType>&& result,
                                Nullable<ErrorType>&& error)
         : _result(result), _error(error)
        {}

        Nullable<ResultType> _result;
        Nullable<ErrorType> _error;
    };
}
#endif
