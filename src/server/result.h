/*
    Copyright (C) 2021, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_SERVER_RESULT_H
#define PMP_SERVER_RESULT_H

#include <QtGlobal>

namespace PMP
{
    enum class ResultCode
    {
        Success = 0,

        NotLoggedIn,

        HashIsNull,

        QueueEntryIdNotFound,
        QueueIndexOutOfRange,
        QueueMaxSizeExceeded,

        InternalError,
    };

    class Result
    {
    public:
        Result(Result const& result) = default;
        Result(Result&& result) = default;
        ~Result() = default;

        ResultCode code() const { return _code; }
        qint64 intArg() const { return _intArg; }

        bool notSuccessful() const { return _code != ResultCode::Success; }

        Result& operator=(Result const& result) = default;
        Result& operator=(Result&& result) = default;

        operator bool() const { return !notSuccessful(); }
        bool operator!() const { return notSuccessful(); }

        bool operator==(Result const& other) { return _code == other._code; }
        bool operator!=(Result const& other) { return !(*this == other); }

    protected:
        Result(ResultCode code)
         : _code(code), _intArg(0)
        {
            //
        }

        Result(ResultCode code, qint64 intArg)
         : _code(code), _intArg(intArg)
        {
            //
        }

    private:
        ResultCode _code;
        qint64 _intArg;
    };

    class Success : public Result
    {
    public:
        Success() : Result(ResultCode::Success) {}
    };

    class Error : public Result
    {
    public:
        static Error notLoggedIn() { return ResultCode::NotLoggedIn; }

        static Error hashIsNull() { return ResultCode::HashIsNull; }

        static Error queueEntryIdNotFound(uint id)
        {
            return Error(ResultCode::QueueEntryIdNotFound, id);
        }

        static Error queueIndexOutOfRange() { return ResultCode::QueueIndexOutOfRange; }
        static Error queueMaxSizeExceeded() { return ResultCode::QueueMaxSizeExceeded; }

        static Error internalError() { return ResultCode::InternalError; }

    private:
        Error(ResultCode code) : Result(code) {}
        Error(ResultCode code, qint64 intArg) : Result(code, intArg) {}
    };

    inline bool succeeded(Result const& result)
    {
        return bool(result);
    }
}
#endif
