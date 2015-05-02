/*
    Copyright (C) 2015, Kevin Andre <hyperquantum@gmail.com>

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

#include "useraccountcreationwidget.h"
#include "ui_useraccountcreationwidget.h"

#include "common/networkprotocol.h"
//#include "common/serverconnection.h"  already included in the header file :-/

#include <QMessageBox>

namespace PMP {

    UserAccountCreationWidget::UserAccountCreationWidget(QWidget *parent,
                                                         ServerConnection* connection)
        : QWidget(parent),
        _ui(new Ui::UserAccountCreationWidget), _connection(connection)
    {
        _ui->setupUi(this);
        _ui->passwordFeedbackLabel->setText(""); /* remove placeholder text */

        connect(
            _ui->newPasswordLineEdit, &QLineEdit::textChanged,
            this, &UserAccountCreationWidget::passwordTextChanged
        );
        connect(
            _ui->createAccountButton, &QPushButton::clicked,
            this, &UserAccountCreationWidget::createAccountClicked
        );
        connect(
            _ui->cancelButton, &QPushButton::clicked,
            this, &UserAccountCreationWidget::cancelClicked
        );

        connect(
            _connection, &ServerConnection::userAccountCreatedSuccessfully,
            this, &UserAccountCreationWidget::userAccountCreatedSuccessfully
        );

        connect(
            _connection, &ServerConnection::userAccountCreationError,
            this, &UserAccountCreationWidget::userAccountCreationError
        );
    }

    UserAccountCreationWidget::~UserAccountCreationWidget()
    {
        delete _ui;
    }

    void UserAccountCreationWidget::passwordTextChanged(const QString &text) {
        QString password = _ui->newPasswordLineEdit->text();

        QString feedback;
        if (password == "") {
            feedback = "";
        }
        else {
            int passwordRating = NetworkProtocol::ratePassword(password);

            feedback = QString("Password score is %1.").arg(passwordRating);
        }

        _ui->passwordFeedbackLabel->setText(feedback);
    }

    void UserAccountCreationWidget::createAccountClicked() {
        QString accountName = _ui->usernameLineEdit->text();
        QString trimmedAccountName = accountName.trimmed();

        if (trimmedAccountName != accountName) {
            QMessageBox::warning(
                this, "Invalid username", "Username cannot start or end with whitespace!"
            );
            return;
        }

        if (accountName.size() == 0) {
            QMessageBox::warning(
                this, "Invalid username", "Username cannot be empty!"
            );
            return;
        }

        if (accountName.size() > 63) {
            QMessageBox::warning(
                this, "Invalid username", "Username is too long!"
            );
            return;
        }

        QString password = _ui->newPasswordLineEdit->text();
        QString retypedPassword = _ui->retypePasswordLineEdit->text();
        if (password == "") {
            QMessageBox::warning(
                this, "Specify password", "Please specify a password!"
            );
            return;
        }
        if (retypedPassword == "") {
            QMessageBox::warning(
                this, "Specify password", "Please retype your password!"
            );
            return;
        }

        if (password != retypedPassword) {
            QMessageBox::warning(
                this, "Invalid password", "Passwords do not match!"
            );
            return;
        }

        int passwordRating = NetworkProtocol::ratePassword(password);
        if (passwordRating <= 20) {
            QMessageBox::warning(
                this, "Bad password",
                QString("Password is too simple! (Score is %1, but should be at least 20)")
                    .arg(QString::number(passwordRating))
            );
            return;
        }

        /* disable input fields */
        _ui->usernameLineEdit->setEnabled(false);
        _ui->newPasswordLineEdit->setEnabled(false);
        _ui->retypePasswordLineEdit->setEnabled(false);
        _ui->createAccountButton->setEnabled(false);

        _connection->createNewUserAccount(accountName, password);
    }

    void UserAccountCreationWidget::userAccountCreatedSuccessfully(QString login,
                                                                   quint32 id)
    {
        QString password = _ui->newPasswordLineEdit->text();
        emit accountCreated(login, password, id);
    }

    void UserAccountCreationWidget::userAccountCreationError(QString login,
                                        ServerConnection::UserRegistrationError errorType)
    {
        QString message;

        switch (errorType) {
        case ServerConnection::AccountAlreadyExists:
            message = "An account with the same name already exists on the server!";
            break;
        case ServerConnection::InvalidAccountName:
            message = "The account name is not valid.";
            break;
        case ServerConnection::UnknownUserRegistrationError:
        default:
            message =
                "An unknown error occurred on the server while trying to register the account!";
            break;
        }

        QMessageBox::warning(this, "Error", message);

        /* reenable input fields */
        _ui->usernameLineEdit->setEnabled(true);
        _ui->newPasswordLineEdit->setEnabled(true);
        _ui->retypePasswordLineEdit->setEnabled(true);
        _ui->createAccountButton->setEnabled(true);
    }
}
