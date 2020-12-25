/*
    Copyright (C) 2015-2020, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_LOGINWIDGET_H
#define PMP_LOGINWIDGET_H

#include "common/userloginerror.h"

#include <QWidget>

namespace Ui {
class LoginWidget;
}

namespace PMP {

    class ServerConnection;

    class LoginWidget : public QWidget
    {
        Q_OBJECT

    public:
        LoginWidget(QWidget* parent, ServerConnection *connection, QString login);
        ~LoginWidget();

    Q_SIGNALS:
        void loggedIn(QString login, quint32 accountId);
        void cancelClicked();

    private Q_SLOTS:
        void loginClicked();

        void userLoggedInSuccessfully(QString login, quint32 id);
        void userLoginError(QString login, UserLoginError errorType);

    private:
        Ui::LoginWidget* _ui;
        ServerConnection* _connection;
    };
}
#endif
