/*
    Copyright (C) 2014, Kevin Andre <hyperquantum@gmail.com>

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

#include "connectionwidget.h"

#include "ui_ConnectionWidget.h"

namespace PMP {

    ConnectionWidget::ConnectionWidget(QWidget* parent)
     : QWidget(parent), _ui(new Ui::ConnectionWidget())
    {
        _ui->setupUi(this);

        _ui->serverLineEdit->setText("localhost");
        _ui->portLineEdit->setText("23432");

        connect(_ui->connectButton, SIGNAL(clicked()), this, SIGNAL(connectClicked()));
    }

    ConnectionWidget::~ConnectionWidget() {
        delete _ui;
    }

//    void ConnectionWidget::connectClicked() {
//
//    }

}
