/*
    Copyright (C) 2021-2025, Kevin André <hyperquantum@gmail.com>

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

#include "common/future.h"

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
        HashIsUnknown,
        TrackIdIsZero,
        TrackIdIsUnknown,

        QueueEntryIdNotFound,
        QueueIndexOutOfRange,
        QueueMaxSizeExceeded,
        QueueItemTypeInvalid,
        DelayOutOfRange,
        LabelNameInvalid,
        LabelIdNotFound,

        UserIdNotFound,

        ScrobblingSystemDisabled,
        ScrobblingProviderInvalid,
        ScrobblingProviderNotEnabled,
        ScrobblingAuthenticationFailed,
        UnspecifiedScrobblingBackendError,

        DatabaseUnavailable,

        NotImplementedError,
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
        static Error notLoggedIn() { return Error(ResultCode::NotLoggedIn); }

        static Error operationAlreadyRunning()
        {
            return Error(ResultCode::OperationAlreadyRunning);
        }

        static Error hashIsNull() { return Error(ResultCode::HashIsNull); }
        static Error hashIsUnknown() { return Error(ResultCode::HashIsUnknown); }

        static Error trackIdIsZero() { return Error(ResultCode::TrackIdIsZero); }
        static Error trackIdIsUnknown() { return Error(ResultCode::TrackIdIsUnknown); }

        static Error queueEntryIdNotFound(uint id)
        {
            return Error(ResultCode::QueueEntryIdNotFound, id);
        }

        static Error queueIndexOutOfRange()
        {
            return Error(ResultCode::QueueIndexOutOfRange);
        }

        static Error queueMaxSizeExceeded()
        {
            return Error(ResultCode::QueueMaxSizeExceeded);
        }

        static Error queueItemTypeInvalid()
        {
            return Error(ResultCode::QueueItemTypeInvalid);
        }

        static Error delayOutOfRange() { return Error(ResultCode::DelayOutOfRange); }
        static Error labelNameInvalid() { return Error(ResultCode::LabelNameInvalid); }

        static Error labelIdNotFound(uint id)
        {
            return Error(ResultCode::LabelIdNotFound, id);
        }

        static Error userIdNotFound() { return Error(ResultCode::UserIdNotFound); }

        static Error scrobblingSystemDisabled()
        {
            return Error(ResultCode::ScrobblingSystemDisabled);
        }

        static Error scrobblingProviderInvalid()
        {
            return Error(ResultCode::ScrobblingProviderInvalid);
        }

        static Error scrobblingProviderNotEnabled()
        {
            return Error(ResultCode::ScrobblingProviderNotEnabled);
        }

        static Error scrobblingAuthenticationFailed()
        {
            return Error(ResultCode::ScrobblingAuthenticationFailed);
        }

        static Error unspecifiedScrobblingBackendError()
        {
            return Error(ResultCode::UnspecifiedScrobblingBackendError);
        }

        static Error databaseUnvailable()
        {
            return Error(ResultCode::DatabaseUnavailable);
        }

        static Error notImplemented() { return Error(ResultCode::NotImplementedError); }
        static Error internalError() { return Error(ResultCode::InternalError); }

        operator SimpleFuture<Result>() const
        {
            return FutureResult<Result>(*this);
        }

        template <class TResult>
        operator Future<TResult, Error>() const
        {
            return FutureError(*this);
        }

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
