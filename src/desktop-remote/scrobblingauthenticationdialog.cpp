/*
    Copyright (C) 2023-2024, Kevin Andre <hyperquantum@gmail.com>

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

#include "scrobblingauthenticationdialog.h"
#include "ui_scrobblingauthenticationdialog.h"

#include "client/scrobblingcontroller.h"
#include "client/serverinterface.h"

#include <QMessageBox>

namespace PMP
{
    ScrobblingAuthenticationDialog::ScrobblingAuthenticationDialog(QWidget* parent,
                                                Client::ServerInterface* serverInterface)
     : QDialog(parent),
       _ui(new Ui::ScrobblingAuthenticationDialog),
       _serverInterface(serverInterface)
    {
        _ui->setupUi(this);

        connect(_ui->usernameLineEdit, &QLineEdit::textChanged,
                this, &ScrobblingAuthenticationDialog::enableDisableWidgets);

        connect(_ui->passwordLineEdit, &QLineEdit::textChanged,
                this, &ScrobblingAuthenticationDialog::enableDisableWidgets);

        connect(_ui->authenticateButton, &QPushButton::clicked,
                this, &ScrobblingAuthenticationDialog::authenticateButtonClicked);

        enableDisableWidgets();
    }

    ScrobblingAuthenticationDialog::~ScrobblingAuthenticationDialog()
    {
        delete _ui;
    }

    void ScrobblingAuthenticationDialog::enableDisableWidgets()
    {
        _ui->usernameLineEdit->setEnabled(!_busy);
        _ui->passwordLineEdit->setEnabled(!_busy);

        _ui->authenticateButton->setEnabled(!_busy
                                            && !_ui->usernameLineEdit->text().isEmpty()
                                            && !_ui->passwordLineEdit->text().isEmpty());
    }

    void ScrobblingAuthenticationDialog::authenticateButtonClicked()
    {
        auto& scrobblingController = _serverInterface->scrobblingController();

        auto username = _ui->usernameLineEdit->text().trimmed();
        auto password = _ui->passwordLineEdit->text();

        if (username.isEmpty())
        {
            QMessageBox::warning(this, tr("Scrobbling"),
                                 tr("Username is required."));
            return;
        }

        if (password.isEmpty())
        {
            QMessageBox::warning(this, tr("Scrobbling"),
                                 tr("Password is required."));
            return;
        }

        _busy = true;
        enableDisableWidgets();

        scrobblingController
            .authenticateLastFm(username, password)
            .handleOnEventLoop(
                this,
                [this](AnyResultMessageCode resultCode)
                {
                    _busy = false;
                    enableDisableWidgets();

                    if (succeeded(resultCode))
                    {
                        QMessageBox::information(this, tr("Scrobbling"),
                                                 tr("Authentication successful."));

                        done(Accepted);
                        return;
                    }

                    if (resultCode ==
                            ScrobblingResultMessageCode::ScrobblingAuthenticationFailed)
                    {
                        QMessageBox::warning(this, tr("Scrobbling"),
                                             tr("Username/password not accepted."));

                        return;
                    }

                    auto failureDetail =
                        tr("Unspecified error (code %1).")
                            .arg(errorCodeString(resultCode));

                    QMessageBox::warning(this, tr("Scrobbling"), failureDetail);
                }
            );
    }
}
