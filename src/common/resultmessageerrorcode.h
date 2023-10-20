/*
    Copyright (C) 2015-2023, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_RESULTMESSAGEERRORCODE_H
#define PMP_RESULTMESSAGEERRORCODE_H

#include <QString>
#include <QtGlobal>

#include <variant>

namespace PMP
{
    enum class ResultMessageErrorCode
    {
        NoError = 0,
        InvalidMessageStructure = 1,
        AlreadyDone = 2 /**< The action was successful but had no effect */,
        NotLoggedIn = 10,

        /// This was historically used for authentication failure (when account not
        /// found), but should be used for account creation only.
        InvalidUserAccountName = 11,

        UserAccountAlreadyExists = 12,
        UserAccountRegistrationMismatch = 13,
        UserAccountLoginMismatch = 14,
        UserLoginAuthenticationFailed = 15,
        AlreadyLoggedIn = 16,

        QueueIdNotFound = 20,
        UnknownAction = 21,
        InvalidHash = 22,
        InvalidQueueIndex = 23,
        InvalidQueueItemType = 24,
        InvalidTimeSpan = 25,

        MaximumQueueSizeExceeded = 50,
        OperationAlreadyRunning = 51,

        DatabaseProblem = 90,

        /// The server does not support the requested action because it is too old.
        /// This error code will probably only ever be used client-side.
        ServerTooOld = 240,

        /// The server does not support the requested action because the protocol
        /// extension the action is a part of is not supported by the server.
        /// This error code will probably only ever be used client-side.
        ExtensionNotSupported = 241,

        /// The action could not be completed because the connection to the server was
        /// broken before the action could be completed.
        /// This error code will probably only ever be used client-side.
        ConnectionToServerBroken = 242,

        NonFatalInternalServerError = 254,
        UnknownError = 255
    };

    inline constexpr bool succeeded(ResultMessageErrorCode errorCode)
    {
        return errorCode == ResultMessageErrorCode::NoError
               || errorCode == ResultMessageErrorCode::AlreadyDone;
    }

    inline QString errorCodeString(ResultMessageErrorCode errorCode)
    {
        return QString("GE%1").arg(static_cast<int>(errorCode));
    }

    enum class ScrobblingResultMessageCode : quint8
    {
        NoError = 0,
        ScrobblingSystemDisabled = 1,
        ScrobblingProviderInvalid = 2,
        ScrobblingProviderNotEnabled = 3,
        ScrobblingAuthenticationFailed = 4,
        UnspecifiedScrobblingBackendError = 5,
    };

    inline constexpr bool succeeded(ScrobblingResultMessageCode code)
    {
        return code == ScrobblingResultMessageCode::NoError;
    }

    inline QString errorCodeString(ScrobblingResultMessageCode code)
    {
        return QString("SC%1").arg(static_cast<int>(code));
    }

    typedef std::variant<ResultMessageErrorCode,
                         ScrobblingResultMessageCode> AnyResultMessageCode;

    inline constexpr bool succeeded(AnyResultMessageCode code)
    {
        if (std::holds_alternative<ResultMessageErrorCode>(code))
            return succeeded(std::get<ResultMessageErrorCode>(code));

        if (std::holds_alternative<ScrobblingResultMessageCode>(code))
            return succeeded(std::get<ScrobblingResultMessageCode>(code));

        Q_UNREACHABLE();
    }

    inline QString errorCodeString(AnyResultMessageCode code)
    {
        if (std::holds_alternative<ResultMessageErrorCode>(code))
            return errorCodeString(std::get<ResultMessageErrorCode>(code));

        if (std::holds_alternative<ScrobblingResultMessageCode>(code))
            return errorCodeString(std::get<ScrobblingResultMessageCode>(code));

        Q_UNREACHABLE();
    }

    inline constexpr bool operator==(AnyResultMessageCode anyCode,
                                     ResultMessageErrorCode errorCode)
    {
        if (errorCode == ResultMessageErrorCode::NoError)
            return succeeded(anyCode);

        if (std::holds_alternative<ResultMessageErrorCode>(anyCode))
            return std::get<ResultMessageErrorCode>(anyCode) == errorCode;

        return false;
    }

    inline constexpr bool operator==(AnyResultMessageCode anyCode,
                                     ScrobblingResultMessageCode errorCode)
    {
        return std::holds_alternative<ScrobblingResultMessageCode>(anyCode)
               && std::get<ScrobblingResultMessageCode>(anyCode) == errorCode;
    }
}
#endif
