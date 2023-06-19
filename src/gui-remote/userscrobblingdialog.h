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

#ifndef PMP_USERSCROBBLINGDIALOG_H
#define PMP_USERSCROBBLINGDIALOG_H

#include "common/nullable.h"
#include "common/scrobblerstatus.h"

#include <QDialog>
#include <QString>

namespace PMP::Client
{
    class ServerInterface;
}

namespace PMP
{
    namespace Ui
    {
        class UserScrobblingDialog;
    }

    class UserScrobblingDialog : public QDialog
    {
        Q_OBJECT

    public:
        explicit UserScrobblingDialog(QWidget* parent,
                                      Client::ServerInterface* serverInterface);
        ~UserScrobblingDialog();

    private:
        void enableDisableButtons();
        void updateStatusLabel();
        QString getStatusText(Nullable<bool> enabled, ScrobblerStatus status);

        Ui::UserScrobblingDialog* _ui;
        Client::ServerInterface* _serverInterface;
    };
}
#endif
