/*
    Copyright (C) 2023, Kevin Andre <hyperquantum@gmail.com>

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

#include "userscrobblingdialog.h"
#include "ui_userscrobblingdialog.h"

#include "client/scrobblingcontroller.h"
#include "client/serverinterface.h"

namespace PMP
{
    UserScrobblingDialog::UserScrobblingDialog(QWidget *parent,
                                               Client::ServerInterface* serverInterface)
     : QDialog(parent, Qt::WindowTitleHint | Qt::WindowCloseButtonHint),
       _ui(new Ui::UserScrobblingDialog),
       _serverInterface(serverInterface)
    {
        _ui->setupUi(this);

        _ui->lastfmAuthenticateButton->setVisible(false); // temporary

        auto* scrobblingController = &_serverInterface->scrobblingController();

        connect(scrobblingController, &Client::ScrobblingController::lastFmInfoChanged,
                this, [this]() { enableDisableButtons(); updateStatusLabel(); });

        connect(
            _ui->lastfmEnableButton, &QPushButton::clicked,
            this,
            [scrobblingController]()
            {
                scrobblingController->setLastFmScrobblingEnabled(true);
            }
        );
        connect(
            _ui->lastfmDisableButton, &QPushButton::clicked,
            this,
            [scrobblingController]()
            {
                scrobblingController->setLastFmScrobblingEnabled(false);
            }
        );
        /*
        connect(
            _ui->lastfmAuthenticateButton, &QPushButton::clicked,
            this,
            [this, scrobblingController]()
            {
                // TODO : show authentication dialog
            }
        );
        */
        enableDisableButtons();
        updateStatusLabel();
    }

    UserScrobblingDialog::~UserScrobblingDialog()
    {
        delete _ui;
    }

    void UserScrobblingDialog::enableDisableButtons()
    {
        auto& scrobblingController = _serverInterface->scrobblingController();

        auto lastFmEnabled = scrobblingController.lastFmEnabled();
        auto lastFmStatus = scrobblingController.lastFmStatus();

        bool canAuthenticate =
            lastFmEnabled == true
                && lastFmStatus == ScrobblerStatus::WaitingForUserCredentials;

        _ui->lastfmEnableButton->setEnabled(lastFmEnabled == false);
        _ui->lastfmDisableButton->setEnabled(lastFmEnabled == true);
        _ui->lastfmAuthenticateButton->setEnabled(canAuthenticate);
    }

    void UserScrobblingDialog::updateStatusLabel()
    {
        auto& scrobblingController = _serverInterface->scrobblingController();

        auto lastFmEnabled = scrobblingController.lastFmEnabled();
        auto lastFmStatus = scrobblingController.lastFmStatus();

        _ui->lastfmStatusValueLabel->setText(getStatusText(lastFmEnabled, lastFmStatus));
    }

    QString UserScrobblingDialog::getStatusText(Nullable<bool> enabled,
                                                ScrobblerStatus status)
    {
        if (enabled == false)
            return tr("disabled");

        switch (status)
        {
        case ScrobblerStatus::Unknown: return tr("unknown");
        case ScrobblerStatus::Green:   return tr("good");
        case ScrobblerStatus::Yellow:  return tr("trying...");
        case ScrobblerStatus::Red:     return tr("BROKEN");
        case ScrobblerStatus::WaitingForUserCredentials:
            return tr("authentication needed");
        }

        qWarning() << "Scrobbler status not recognized:" << status;
        return tr("unknown");
    }
}
