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

#ifndef PMP_USERPICKERWIDGET_H
#define PMP_USERPICKERWIDGET_H

#include "common/serverhealthstatus.h"

#include <QList>
#include <QPair>
#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QCommandLinkButton)

namespace Ui {
    class UserPickerWidget;
}

namespace PMP {

    class ServerConnection;

    class UserPickerWidget : public QWidget {
        Q_OBJECT

    public:
        UserPickerWidget(QWidget *parent, ServerConnection* connection);
        ~UserPickerWidget();

    Q_SIGNALS:
        void accountClicked(QString login);
        void createAccountClicked();

    private Q_SLOTS:
        void receivedUserAccounts(QList<QPair<uint, QString> > accounts);
        void serverHealthChanged(ServerHealthStatus serverHealth);

    private:
        Ui::UserPickerWidget* _ui;
        ServerConnection* _connection;
        bool _serverProblemsPreventLogin;
    };
}
#endif
