/*
    Copyright (C) 2015-2021, Kevin Andre <hyperquantum@gmail.com>

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

namespace PMP
{
    enum class ResultMessageErrorCode
    {
        NoError = 0,
        InvalidMessageStructure = 1,
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

        DatabaseProblem = 90,
        NonFatalInternalServerError = 254,
        UnknownError = 255
    };

    inline bool succeeded(ResultMessageErrorCode errorCode)
    {
        return errorCode == ResultMessageErrorCode::NoError;
    }
}
#endif
