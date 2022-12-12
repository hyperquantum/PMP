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

#include "authenticationcontrollerimpl.h"

#include "serverconnection.h"

namespace PMP::Client
{
    AuthenticationControllerImpl::AuthenticationControllerImpl(
                                                             ServerConnection* connection)
     : AuthenticationController(connection),
       _connection(connection)
    {
        connect(
            _connection, &ServerConnection::connected,
            this, &AuthenticationControllerImpl::connected
        );
        connect(
            _connection, &ServerConnection::disconnected,
            this, &AuthenticationControllerImpl::connectionBroken
        );

        connect(
            _connection, &ServerConnection::userAccountsReceived,
            this, &AuthenticationControllerImpl::userAccountsReceived
        );
        connect(
            _connection, &ServerConnection::userAccountCreatedSuccessfully,
            this, &AuthenticationControllerImpl::userAccountCreatedSuccessfully
        );
        connect(
            _connection, &ServerConnection::userAccountCreationError,
            this, &AuthenticationControllerImpl::userAccountCreationError
        );

        connect(
            _connection, &ServerConnection::userLoggedInSuccessfully,
            this, &AuthenticationControllerImpl::userLoggedInSuccessfully
        );
        connect(
            _connection, &ServerConnection::userLoginError,
            this, &AuthenticationControllerImpl::userLoginFailed
        );
    }

    void AuthenticationControllerImpl::sendUserAccountsFetchRequest()
    {
        _connection->sendUserAccountsFetchRequest();
    }

    void AuthenticationControllerImpl::createNewUserAccount(QString login,
                                                            QString password)
    {
        _connection->createNewUserAccount(login, password);
    }

    void AuthenticationControllerImpl::login(QString login, QString password)
    {
        _connection->login(login, password);
    }

    bool AuthenticationControllerImpl::isLoggedIn() const
    {
        return _connection->isLoggedIn();
    }

    quint32 AuthenticationControllerImpl::userLoggedInId() const
    {
        return _connection->userLoggedInId();
    }

    QString AuthenticationControllerImpl::userLoggedInName() const
    {
        return _connection->userLoggedInName();
    }

    void AuthenticationControllerImpl::connected()
    {
        //
    }

    void AuthenticationControllerImpl::connectionBroken()
    {
        //
    }

}
