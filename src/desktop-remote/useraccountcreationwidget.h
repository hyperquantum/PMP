/*
    Copyright (C) 2015-2022, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_USERACCOUNTCREATIONWIDGET_H
#define PMP_USERACCOUNTCREATIONWIDGET_H

#include "common/passwordstrengthevaluator.h"
#include "common/userregistrationerror.h"

#include <QString>
#include <QWidget>

namespace Ui
{
    class UserAccountCreationWidget;
}

namespace PMP::Client
{
    class AuthenticationController;
}

namespace PMP
{
    class UserAccountCreationWidget : public QWidget
    {
        Q_OBJECT
    public:
        UserAccountCreationWidget(QWidget* parent,
                              Client::AuthenticationController* authenticationController);
        ~UserAccountCreationWidget();

    Q_SIGNALS:
        void accountCreated(QString login, QString password, quint32 accountId);
        void cancelClicked();

    private Q_SLOTS:
        void passwordTextChanged(QString const& text);
        void createAccountClicked();

        void userAccountCreatedSuccessfully(QString login, quint32 id);
        void userAccountCreationError(QString login, UserRegistrationError errorType);

    private:
        QString ratingToString(PasswordStrengthRating rating);

        Ui::UserAccountCreationWidget* _ui;
        Client::AuthenticationController* _authenticationController;
    };
}
#endif
