/*
    Copyright (C) 2014-2016, Kevin Andre <hyperquantum@gmail.com>

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

#include "ui_connectionwidget.h"

#include "common/serverdiscoverer.h"

#include <QCommandLinkButton>
#include <QMessageBox>

namespace PMP {

    ConnectionWidget::ConnectionWidget(QWidget* parent)
     : QWidget(parent), _ui(new Ui::ConnectionWidget())
    {
        _ui->setupUi(this);

        _ui->serverLineEdit->setText("localhost");

        QIntValidator* portValidator = new QIntValidator(1, 65535, this);
        _ui->portLineEdit->setValidator(portValidator);
        _ui->portLineEdit->setText("23432");

        connect(_ui->connectButton, SIGNAL(clicked()), this, SLOT(connectClicked()));

        auto discoverer = new ServerDiscoverer(this);

        connect(
            discoverer, &ServerDiscoverer::foundServer,
            this, &ConnectionWidget::foundServer
        );

        discoverer->sendProbe();
    }

    ConnectionWidget::~ConnectionWidget() {
        delete _ui;
    }

    void ConnectionWidget::reenableFields() {
        _ui->serverLineEdit->setEnabled(true);
        _ui->portLineEdit->setEnabled(true);
        _ui->connectButton->setEnabled(true);
    }

    void ConnectionWidget::foundServer(QHostAddress address, quint16 port,
                                       QUuid id, QString name)
    {
        QCommandLinkButton* button =
            new QCommandLinkButton(_ui->serversFoundGroupBox);

        QString caption = name;
        if (caption == "") { caption = tr("<server with unknown name>"); }

        button->setText(caption);
        button->setDescription(address.toString());
        // TODO: add extra adresses if they appear
        button->adjustSize();

        connect(
            button, &QCommandLinkButton::clicked,
            [=]() {
                _ui->serverLineEdit->setEnabled(false);
                _ui->portLineEdit->setEnabled(false);
                _ui->connectButton->setEnabled(false);

                emit doConnect(address.toString(), port);
            }
        );

        _ui->serversFoundGroupBox->layout()->addWidget(button);

        _ui->noServersFoundLabel->hide();
    }

    void ConnectionWidget::connectClicked() {
        QString server = _ui->serverLineEdit->text().trimmed();
        QString portString = _ui->portLineEdit->text().trimmed();

        QMessageBox msg;

        /* Validate server */
        if (server.length() == 0) {
            msg.setText("You need to fill in the hostname or IP of the server!");
            msg.exec();
            return;
        }

        /* Validate port number */
        int pos = 0;
        if (_ui->portLineEdit->validator()->validate(portString, pos) != QValidator::Acceptable) {
            msg.setText("Invalid port number!");
            msg.setInformativeText("Port number must be in the range 1 to 65535.");
            msg.exec();
            return;
        }
        uint portNumber = portString.toUInt();

        _ui->serverLineEdit->setEnabled(false);
        _ui->portLineEdit->setEnabled(false);
        _ui->connectButton->setEnabled(false);

        emit doConnect(server, portNumber);
    }

}
