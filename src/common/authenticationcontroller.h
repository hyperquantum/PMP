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

#ifndef PMP_AUTHENTICATIONCONTROLLER_H
#define PMP_AUTHENTICATIONCONTROLLER_H

#include "userloginerror.h"

#include <QObject>

namespace PMP
{
    class AuthenticationController : public QObject
    {
        Q_OBJECT
    public:
        virtual ~AuthenticationController() {}

        virtual void login(QString login, QString password) = 0;

        virtual bool isLoggedIn() const = 0;
        virtual quint32 userLoggedInId() const = 0;
        virtual QString userLoggedInName() const = 0;

    Q_SIGNALS:
        void userLoggedInSuccessfully(QString login, quint32 id);
        void userLoginFailed(QString login, UserLoginError errorType);

    protected:
        explicit AuthenticationController(QObject* parent) : QObject(parent) {}
    };
}
#endif
