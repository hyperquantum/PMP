/*
    Copyright (C) 2015-2016, Kevin Andre <hyperquantum@gmail.com>

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

#include "loginwidget.h"
#include "ui_loginwidget.h"

#include "common/networkprotocol.h"
//#include "common/serverconnection.h"  already included in the header file :-/

#include <QMessageBox>

namespace PMP {

    LoginWidget::LoginWidget(QWidget* parent, ServerConnection *connection,
                             QString login) :
        QWidget(parent),
        _ui(new Ui::LoginWidget), _connection(connection)
    {
        _ui->setupUi(this);

        if (!login.isEmpty()) {
            _ui->usernameLineEdit->setText(login);
            _ui->passwordLineEdit->setFocus();
        }
        else {
            _ui->usernameLineEdit->setFocus();
        }

        connect(
            _ui->passwordLineEdit, &QLineEdit::returnPressed,
            this, &LoginWidget::loginClicked
        );
        connect(
            _ui->loginButton, &QPushButton::clicked,
            this, &LoginWidget::loginClicked
        );
        connect(
            _ui->cancelButton, &QPushButton::clicked,
            this, &LoginWidget::cancelClicked
        );

        connect(
            _connection, &ServerConnection::userLoggedInSuccessfully,
            this, &LoginWidget::userLoggedInSuccessfully
        );
        connect(
            _connection, &ServerConnection::userLoginError,
            this, &LoginWidget::userLoginError
        );
    }

    LoginWidget::~LoginWidget()
    {
        delete _ui;
    }

    void LoginWidget::loginClicked() {
        QString accountName = _ui->usernameLineEdit->text();

        if (accountName.size() == 0) {
            QMessageBox::warning(
                this, "Missing username", "Please specify username!"
            );
            return;
        }

        if (accountName.size() > 63) {
            QMessageBox::warning(
                this, "Invalid username", "Username is too long!"
            );
            return;
        }

        QString password = _ui->passwordLineEdit->text();

        /* disable input fields */
        _ui->usernameLineEdit->setEnabled(false);
        _ui->passwordLineEdit->setEnabled(false);
        _ui->loginButton->setEnabled(false);

        _connection->login(accountName, password);
    }

    void LoginWidget::userLoggedInSuccessfully(QString login, quint32 id) {
        emit loggedIn(login, id);
    }

    void LoginWidget::userLoginError(QString login,
                                     ServerConnection::UserLoginError errorType)
    {
        QString message;

        switch (errorType) {
        case ServerConnection::UserLoginAuthenticationFailed:
            message = "The specified user/password combination is not valid.";
            break;
        default:
            message =
                "An unknown error occurred on the server while trying to login!";
            break;
        }

        QMessageBox::warning(this, "Error", message);

        /* reenable input fields */
        _ui->usernameLineEdit->setEnabled(true);
        _ui->passwordLineEdit->setEnabled(true);
        _ui->loginButton->setEnabled(true);
    }
}
