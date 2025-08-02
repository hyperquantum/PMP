/*
    Copyright (C) 2015-2025, Kevin André <hyperquantum@gmail.com>

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
        NoError = 0, /**< The action was successful */
        InvalidMessageStructure = 1, /**< The message could not be parsed correctly */
        AlreadyDone = 2 /**< The action was successful but had no effect */,

        /* ---------- errors regarding authentication ---------- */

        NotLoggedIn = 10, /**< The request requires authentication first */

        /// This was historically used for authentication failure (when account not
        /// found), but should be used for account creation only.
        InvalidUserAccountName = 11,

        UserAccountAlreadyExists = 12, /**< The user account already exists */

        /// Inconsistency between different parts of the user account registration
        /// procedure.
        UserAccountRegistrationMismatch = 13,

        /// Inconsistency between different parts of the login procedure
        UserAccountLoginMismatch = 14,

        UserLoginAuthenticationFailed = 15, /**< Login/password combination not valid */
        AlreadyLoggedIn = 16, /**< Cannot authenticate a second time */

        /* ---------- errors regarding an invalid argument ---------- */

        QueueIdNotFound = 20, /**< The specified queue ID could not be found */
        UnknownAction = 21, /**< An unknown action (number) was specified */
        InvalidHash = 22, /**< The specified track hash was not valid */
        InvalidQueueIndex = 23, /**< The specified queue index was not valid */
        InvalidQueueItemType = 24, /**< The specified queue item type was not valid */
        InvalidTimeSpan = 25, /**< The specified time span was not valid */
        InvalidUserId = 26, /**< The specified user ID was not valid */
        InvalidLabelName = 27, /**< The specified label name was not valid */

        /* ---------- errors regarding state ---------- */

        MaximumQueueSizeExceeded = 50, /**< Maximum queue size would be exceeded */

        /// The operation is already running and cannot be started again.
        OperationAlreadyRunning = 51,

        /* ---------- database errors ---------- */

        DatabaseProblem = 90, /**< A database error occurred */

        /* ---------- errors regarding sending a reply ---------- */

        TooMuchDataToReturn = 120, /**< The response to the request would be too large */
        NumberTooBigToReturn = 121, /**< The number to return would be too large */

        /* ---------- errors regarding client-server communication ---------- */

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

        /* ---------- really generic errors ---------- */

        NonFatalInternalServerError = 254, /**< Internal server error, non-fatal */
        UnknownError = 255 /**< Unknown error */
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
