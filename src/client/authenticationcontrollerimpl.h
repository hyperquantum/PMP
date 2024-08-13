/*
    Copyright (C) 2021-2024, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_AUTHENTICATIONCONTROLLERIMPL_H
#define PMP_AUTHENTICATIONCONTROLLERIMPL_H

#include "authenticationcontroller.h"

#include "common/nullable.h"
#include "common/promise.h"

#include <QSharedPointer>

namespace PMP::Client
{
    class ServerConnection;

    class AuthenticationControllerImpl : public AuthenticationController
    {
        Q_OBJECT
    public:
        explicit AuthenticationControllerImpl(ServerConnection* connection);

        Future<QList<UserAccount>, ResultMessageErrorCode> getUserAccounts() override;
        void sendUserAccountsFetchRequest() override;

        void createNewUserAccount(QString login, QString password) override;

        void login(QString login, QString password) override;
        bool isLoggedIn() const override;
        quint32 userLoggedInId() const override;
        QString userLoggedInName() const override;

    private Q_SLOTS:
        void connected();
        void connectionBroken();

    private:
        ServerConnection* _connection;
        Nullable<Promise<QList<UserAccount>, ResultMessageErrorCode>> _userAccountsPromise;
    };
}
#endif
