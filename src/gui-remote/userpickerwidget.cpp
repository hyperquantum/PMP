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

#include "userpickerwidget.h"
#include "ui_userpickerwidget.h"

#include "client/authenticationcontroller.h"
#include "client/generalcontroller.h"

#include <algorithm>

using namespace PMP::Client;

namespace PMP
{
    UserPickerWidget::UserPickerWidget(QWidget* parent,
                                       GeneralController* generalController,
                                       AuthenticationController* authenticationController)
     : QWidget(parent),
       _ui(new Ui::UserPickerWidget),
       _generalController(generalController),
       _authenticationController(authenticationController),
       _serverProblemsPreventLogin(generalController->serverHealth().databaseUnavailable())
    {
        _ui->setupUi(this);

        _ui->noUserAccountsYetLabel->setVisible(false);
        _ui->createNewAccountButton->setEnabled(false);

        connect(
            _generalController, &GeneralController::serverHealthChanged,
            this, [this]() { checkServerHealth(); }
        );

        connect(
            _authenticationController, &AuthenticationController::userAccountsReceived,
            this, &UserPickerWidget::receivedUserAccounts
        );

        connect(
            _ui->createNewAccountButton, &QCommandLinkButton::clicked,
            this, &UserPickerWidget::createAccountClicked
        );

        _authenticationController->sendUserAccountsFetchRequest();

        checkServerHealth();
    }

    UserPickerWidget::~UserPickerWidget()
    {
        delete _ui;
    }

    void UserPickerWidget::receivedUserAccounts(QList<QPair<uint, QString> > accounts)
    {
        bool serverOk = !_serverProblemsPreventLogin;

        std::sort(
            accounts.begin(), accounts.end(),
            [](const QPair<uint,QString>& u1, const QPair<uint,QString>& u2)
            {
                return u1.second < u2.second;
            }
        );

        _ui->loadingUserListLabel->setVisible(false);

        if (accounts.size() <= 0)
        {
            _ui->noUserAccountsYetLabel->setVisible(serverOk);
        }
        else
        {
            for (int i = 0; i < accounts.size(); ++i)
            {
                QString username = accounts[i].second;

                QCommandLinkButton* button =
                    new QCommandLinkButton(_ui->usersListFrame);
                button->setText(username);
                button->setDescription(QString(tr("Login as %1")).arg(username));

                connect(
                    button, &QCommandLinkButton::clicked,
                    this,
                    [this, username]() { Q_EMIT accountClicked(username); }
                );

                _ui->usersListLayout->addWidget(button);
            }
        }

        _ui->createNewAccountButton->setEnabled(serverOk);
    }

    void UserPickerWidget::checkServerHealth()
    {
        auto serverHealth = _generalController->serverHealth();

        _serverProblemsPreventLogin = serverHealth.databaseUnavailable();

        if (_serverProblemsPreventLogin)
        {
            _ui->noUserAccountsYetLabel->setVisible(false);
            _ui->createNewAccountButton->setEnabled(false);
        }
    }

}
