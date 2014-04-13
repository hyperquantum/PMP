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

#include "mainwindow.h"

#include "connectionwidget.h"
#include "mainwidget.h"
#include "serverconnection.h"

namespace PMP {

    MainWindow::MainWindow(QWidget* parent)
     : QMainWindow(parent),
       _connectionWidget(new ConnectionWidget(this)),
       _connection(0), _mainWidget(0)
    {
        setCentralWidget(_connectionWidget);
        connect(_connectionWidget, SIGNAL(doConnect(QString, uint)), this, SLOT(onDoConnect(QString, uint)));
    }

    MainWindow::~MainWindow() {
        //
    }

    void MainWindow::onDoConnect(QString server, uint port) {
        _connection = new ServerConnection();
        connect(_connection, SIGNAL(connected()), this, SLOT(onConnected()));
        _connection->connectToHost(server, port);
    }

    void MainWindow::onConnected() {
        _mainWidget = new MainWidget(this);
        _mainWidget->setConnection(_connection);
        setCentralWidget(_mainWidget);
        _connectionWidget->close();
    }

}
