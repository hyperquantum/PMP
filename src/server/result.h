/*
    Copyright (C) 2021-2022, Kevin Andre <hyperquantum@gmail.com>

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

namespace PMP::Server
{
    enum class ResultCode
    {
        Success = 0,
        NoOp /**< Successful action that had no effect */,

        NotLoggedIn,

        OperationAlreadyRunning,

        HashIsNull,

        QueueEntryIdNotFound,
        QueueIndexOutOfRange,
        QueueMaxSizeExceeded,
        QueueItemTypeInvalid,
        DelayOutOfRange,

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

        bool notSuccessful() const { return _code != ResultCode::Success &&
                                            _code != ResultCode::NoOp; }

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

    class NoOp : public Result
    {
    public:
        NoOp() : Result(ResultCode::NoOp) {}
    };

    class Error : public Result
    {
    public:
        static Result notLoggedIn() { return Error(ResultCode::NotLoggedIn); }

        static Result operationAlreadyRunning()
        {
            return Error(ResultCode::OperationAlreadyRunning);
        }

        static Result hashIsNull() { return Error(ResultCode::HashIsNull); }

        static Result queueEntryIdNotFound(uint id)
        {
            return Error(ResultCode::QueueEntryIdNotFound, id);
        }

        static Result queueIndexOutOfRange()
        {
            return Error(ResultCode::QueueIndexOutOfRange);
        }

        static Result queueMaxSizeExceeded()
        {
            return Error(ResultCode::QueueMaxSizeExceeded);
        }

        static Result queueItemTypeInvalid()
        {
            return Error(ResultCode::QueueItemTypeInvalid);
        }

        static Result delayOutOfRange() { return Error(ResultCode::DelayOutOfRange); }

        static Result internalError() { return Error(ResultCode::InternalError); }

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
