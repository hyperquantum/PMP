/*
    Copyright (C) 2020-2021, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_USERLOGINERROR_H
#define PMP_USERLOGINERROR_H

#include <QtDebug>

namespace PMP
{
    enum class UserLoginError
    {
        UnknownError,
        AuthenticationFailed
    };

    inline QDebug operator<<(QDebug debug, UserLoginError error)
    {
        switch (error)
        {
            case UserLoginError::UnknownError:
                return debug << "UserLoginError::UnknownError";

            case UserLoginError::AuthenticationFailed:
                return debug << "UserLoginError::AuthenticationFailed";
        }

        return debug << int(error);
    }
}

Q_DECLARE_METATYPE(PMP::UserLoginError)

#endif
