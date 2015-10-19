/*
    Copyright (C) 2014-2015, Kevin Andre <hyperquantum@gmail.com>

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

#include "common/serverconnection.h"

#include "connectionwidget.h"
#include "loginwidget.h"
#include "mainwidget.h"
#include "useraccountcreationwidget.h"
#include "userpickerwidget.h"

#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>

namespace PMP {

    MainWindow::MainWindow(QWidget* parent)
     : QMainWindow(parent),
       _connectionWidget(new ConnectionWidget(this)),
       _connection(0), _userPickerWidget(0), _loginWidget(0), _mainWidget(0)
    {
        createMenus();
        updateStatusBar();

        setCentralWidget(_connectionWidget);
        connect(
            _connectionWidget, &ConnectionWidget::doConnect,
            this, &MainWindow::onDoConnect
        );
    }

    MainWindow::~MainWindow() {
        //
    }

    void MainWindow::createMenus() {
        /* Actions */

        _shutdownServerAction = new QAction(tr("&Shutdown server"), this);
        connect(
            _shutdownServerAction, &QAction::triggered,
            this, &MainWindow::onShutdownServerTriggered
        );

        _startFullIndexationAction = new QAction(tr("&Start full indexation"), this);
        _startFullIndexationAction->setVisible(false); /* needs active connection */
        connect(
            _startFullIndexationAction, &QAction::triggered,
            this, &MainWindow::onStartFullIndexationTriggered
        );

        _closeAction = new QAction(tr("&Close remote"), this);
        connect(_closeAction, &QAction::triggered, this, &MainWindow::close);

        /* Menus */

        QMenu* menu = menuBar()->addMenu(tr("&PMP"));

        menu->addAction(_startFullIndexationAction);

        QMenu* serverAdminMenu = menu->addMenu(tr("Server &administration"));
        _serverAdminAction = serverAdminMenu->menuAction();
        _serverAdminAction->setVisible(false); /* needs active connection */

        serverAdminMenu->addAction(_shutdownServerAction);

        menu->addSeparator();
        menu->addAction(_closeAction);
    }

    void MainWindow::updateStatusBar() {
        auto bar = this->statusBar();

        if (!_connection || !_connection->isConnected()) {
            bar->showMessage(tr("Not connected."));
        }
        else if (_connection->userLoggedInId() <= 0) {
            bar->showMessage(tr("Connected."));
        }
        else {
            bar->showMessage(
                QString(tr("Logged in as %1")).arg(_connection->userLoggedInName())
            );
        }
    }

    void MainWindow::onStartFullIndexationTriggered() {
        if (_connection) { _connection->startFullIndexation(); }
    }

    void MainWindow::onShutdownServerTriggered() {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText(tr("You are about to shutdown the PMP server."));
        msgBox.setInformativeText(
            tr("All remotes (clients) connected to this server will be closed,"
               " and the server will become unavailable. "
               "Are you sure you wish to continue?")
        );
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Cancel);
        int buttonClicked = msgBox.exec();

        if (buttonClicked == QMessageBox::Cancel) return;

        _connection->shutdownServer();
    }

    void MainWindow::onDoConnect(QString server, uint port) {
        _connection = new ServerConnection();

        connect(
            _connection, &ServerConnection::connected,
            this, &MainWindow::onConnected
        );
        connect(
            _connection, &ServerConnection::cannotConnect,
            this, &MainWindow::onCannotConnect
        );
        connect(
            _connection, &ServerConnection::invalidServer,
            this, &MainWindow::onInvalidServer
        );
        connect(
            _connection, &ServerConnection::connectionBroken,
            this, &MainWindow::onConnectionBroken
        );
        connect(
            _connection, &ServerConnection::fullIndexationStarted,
            [this] { _startFullIndexationAction->setEnabled(false); }
        );
        connect(
            _connection, &ServerConnection::fullIndexationFinished,
            [this] { _startFullIndexationAction->setEnabled(_connection->isLoggedIn()); }
        );

        _connection->connectToHost(server, port);
    }

    void MainWindow::onConnected() {
        showUserAccountPicker();
        updateStatusBar();
    }

    void MainWindow::showUserAccountPicker() {
        _userPickerWidget = new UserPickerWidget(this, _connection);

        connect(
            _userPickerWidget, &UserPickerWidget::accountClicked,
            this, &MainWindow::showLoginWidget
        );

        connect(
            _userPickerWidget, &UserPickerWidget::createAccountClicked,
            this, &MainWindow::onCreateAccountClicked
        );

        setCentralWidget(_userPickerWidget);
    }

    void MainWindow::onCannotConnect(QAbstractSocket::SocketError error) {
        QMessageBox::warning(
            this, tr("Connection failure"), tr("Failed to connect to that server.")
        );

        /* let the user try to correct any possible mistake */
        _connectionWidget->reenableFields();
    }

    void MainWindow::onInvalidServer() {
        QMessageBox::warning(
            this, tr("Connection failure"), tr("This is not a valid PMP server!")
        );

        /* let the user try to correct any possible mistake */
        _connectionWidget->reenableFields();
    }

    void MainWindow::onConnectionBroken(QAbstractSocket::SocketError error) {
        updateStatusBar();

        QMessageBox::warning(
            this, tr("Connection failure"), tr("Connection to the server was lost!")
        );
        this->close();
    }

    void MainWindow::showMainWidget() {
        _mainWidget = new MainWidget(this);
        _mainWidget->setConnection(_connection);
        setCentralWidget(_mainWidget);
    }

    void MainWindow::onCreateAccountClicked() {
        _userAccountCreationWidget = new UserAccountCreationWidget(this, _connection);

        connect(
            _userAccountCreationWidget, &UserAccountCreationWidget::accountCreated,
            this, &MainWindow::onAccountCreated
        );
        connect(
            _userAccountCreationWidget, &UserAccountCreationWidget::cancelClicked,
            this, &MainWindow::onAccountCreationCancel
        );

        setCentralWidget(_userAccountCreationWidget);
    }

    void MainWindow::onAccountCreated(QString login, QString password, quint32 accountId)
    {
        _userAccountCreationWidget = 0;
        showUserAccountPicker();
    }

    void MainWindow::onAccountCreationCancel() {
        _userAccountCreationWidget = 0;
        showUserAccountPicker();
    }

    void MainWindow::showLoginWidget(QString login) {
        _loginWidget = new LoginWidget(this, _connection, login);

        connect(
            _loginWidget, &LoginWidget::loggedIn,
            this, &MainWindow::onLoggedIn
        );

        connect(
            _loginWidget, &LoginWidget::cancelClicked,
            this, &MainWindow::onLoginCancel
        );

        setCentralWidget(_loginWidget);
    }

    void MainWindow::onLoggedIn(QString login) {
        updateStatusBar();
        _connection->requestFullIndexationRunningStatus();

        _loginWidget = 0;
        showMainWidget();

        _startFullIndexationAction->setEnabled(false);
        _startFullIndexationAction->setVisible(true);
        _serverAdminAction->setVisible(true);
    }

    void MainWindow::onLoginCancel() {
        _loginWidget = 0;
        showUserAccountPicker();
    }
}
