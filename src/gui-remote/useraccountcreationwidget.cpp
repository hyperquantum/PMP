/*
    Copyright (C) 2015-2024, Kevin Andre <hyperquantum@gmail.com>

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

#include "common/passwordstrengthevaluator.h"

#include "client/authenticationcontroller.h"

#include <QtDebug>
#include <QMessageBox>

using namespace PMP::Client;

namespace PMP
{
    UserAccountCreationWidget::UserAccountCreationWidget(QWidget *parent,
                                       AuthenticationController* authenticationController)
     : QWidget(parent),
       _ui(new Ui::UserAccountCreationWidget),
       _authenticationController(authenticationController)
    {
        _ui->setupUi(this);
        _ui->passwordFeedbackLabel->setText(""); /* remove placeholder text */
        _ui->usernameLineEdit->setFocus();

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
            _authenticationController,
            &AuthenticationController::userAccountCreatedSuccessfully,
            this,
            &UserAccountCreationWidget::userAccountCreatedSuccessfully
        );
        connect(
            _authenticationController,
            &AuthenticationController::userAccountCreationError,
            this,
            &UserAccountCreationWidget::userAccountCreationError
        );
    }

    UserAccountCreationWidget::~UserAccountCreationWidget()
    {
        delete _ui;
    }

    void UserAccountCreationWidget::passwordTextChanged(const QString& text)
    {
        Q_UNUSED(text)

        QString password = _ui->newPasswordLineEdit->text();

        QString feedback;
        if (password == "")
        {
            feedback = "";
        }
        else
        {
            auto rating = PasswordStrengthEvaluator::getPasswordRating(password);

            feedback = QString("Password strength: %1").arg(ratingToString(rating));
        }

        _ui->passwordFeedbackLabel->setText(feedback);
    }

    void UserAccountCreationWidget::createAccountClicked()
    {
        QString accountName = _ui->usernameLineEdit->text();
        QString trimmedAccountName = accountName.trimmed();

        if (trimmedAccountName != accountName)
        {
            QMessageBox::warning(
                this, "Invalid username", "Username cannot start or end with whitespace!"
            );
            return;
        }

        if (accountName.size() == 0)
        {
            QMessageBox::warning(
                this, "Invalid username", "Username cannot be empty!"
            );
            return;
        }

        if (accountName.size() > 63)
        {
            QMessageBox::warning(
                this, "Invalid username", "Username is too long!"
            );
            return;
        }

        QString password = _ui->newPasswordLineEdit->text();
        QString retypedPassword = _ui->retypePasswordLineEdit->text();
        if (password == "")
        {
            QMessageBox::warning(
                this, "Specify password", "Please specify a password!"
            );
            return;
        }
        if (retypedPassword == "")
        {
            QMessageBox::warning(
                this, "Specify password", "Please retype your password!"
            );
            return;
        }

        if (password != retypedPassword)
        {
            QMessageBox::warning(
                this, "Invalid password", "Passwords do not match!"
            );
            return;
        }

        auto rating = PasswordStrengthEvaluator::getPasswordRating(password);
        if (rating == PasswordStrengthRating::TooWeak)
        {
            QMessageBox messageBox;
            messageBox.setIcon(QMessageBox::Warning);
            messageBox.setText(
                tr("The password is too weak. Make it longer or more complicated."));
            messageBox.setInformativeText(
                tr("Try using characters from multiple categories:\n"
                   "1. lowercase letters\n"
                   "2. uppercase letters\n"
                   "3. digits\n"
                   "4. special characters"));
            messageBox.exec();

            return;
        }

        /* disable input fields */
        _ui->usernameLineEdit->setEnabled(false);
        _ui->newPasswordLineEdit->setEnabled(false);
        _ui->retypePasswordLineEdit->setEnabled(false);
        _ui->createAccountButton->setEnabled(false);

        _authenticationController->createNewUserAccount(accountName, password);
    }

    void UserAccountCreationWidget::userAccountCreatedSuccessfully(QString login,
                                                                   quint32 id)
    {
        QString password = _ui->newPasswordLineEdit->text();
        Q_EMIT accountCreated(login, password, id);
    }

    void UserAccountCreationWidget::userAccountCreationError(QString login,
                                                          UserRegistrationError errorType)
    {
        Q_UNUSED(login)

        QString message;

        switch (errorType)
        {
        case UserRegistrationError::AccountAlreadyExists:
            message = "An account with the same name already exists on the server!";
            break;

        case UserRegistrationError::InvalidAccountName:
            message = "The account name is not valid.";
            break;

        case UserRegistrationError::UnknownError:
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

    QString UserAccountCreationWidget::ratingToString(PasswordStrengthRating rating)
    {
        switch (rating)
        {
        case PasswordStrengthRating::TooWeak: return tr("too weak");
        case PasswordStrengthRating::Acceptable: return tr("acceptable");
        case PasswordStrengthRating::Good: return tr("good");
        case PasswordStrengthRating::VeryGood: return tr("very good");
        case PasswordStrengthRating::Excellent: return tr("excellent");
        }

        return QString::number(int(rating));
    }
}
