/*
    Copyright (C) 2014-2021, Kevin Andre <hyperquantum@gmail.com>

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

#include "common/clientserverinterface.h"
#include "common/generalcontroller.h"
#include "common/playercontroller.h"
#include "common/powermanagement.h"
#include "common/serverconnection.h"
#include "common/util.h"
#include "common/version.h"

#include "collectionwidget.h"
#include "connectionwidget.h"
#include "loginwidget.h"
#include "mainwidget.h"
#include "useraccountcreationwidget.h"
#include "userpickerwidget.h"

#include <QAction>
#include <QApplication>
#include <QCoreApplication>
#include <QDesktopWidget>
#include <QDockWidget>
#include <QKeyEvent>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QStatusBar>
#include <QTimer>

namespace PMP
{
    MainWindow::MainWindow(QWidget* parent)
     : QMainWindow(parent),
       _leftStatusTimer(new QTimer(this)),
       _connectionWidget(new ConnectionWidget(this)),
       _connection(nullptr), _clientServerInterface(nullptr),
       _userPickerWidget(nullptr), _loginWidget(nullptr), _mainWidget(nullptr),
       _musicCollectionDock(new QDockWidget(tr("Music collection"), this)),
       _powerManagement(new PowerManagement(this))
    {
        setWindowTitle(
            QString(tr("Party Music Player ")) + Util::EnDash + tr(" Remote")
        );

        _musicCollectionDock->setObjectName("musicCollectionDockWidget");
        _musicCollectionDock->setAllowedAreas(
            (Qt::DockWidgetAreas)(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea)
        );

        createMenus();
        createStatusbar();

        setCentralWidget(_connectionWidget);
        connect(
            _connectionWidget, &ConnectionWidget::doConnect,
            this, &MainWindow::onDoConnect
        );

        {
            QSettings settings(QCoreApplication::organizationName(),
                               QCoreApplication::applicationName());

            qDebug() << "Before restore:" << this->pos() << " size:" << this->size();

            settings.beginGroup("mainwindow");
            restoreGeometry(settings.value("geometry").toByteArray());

            // QTBUG-77385
            if (!this->geometry().intersects(
                        QApplication::desktop()->screenGeometry(
                            QApplication::desktop()->screenNumber(this))))
            {
                qWarning() << "Need to apply workaround for QTBUG-77385";
                auto availableGeometry = QApplication::desktop()->availableGeometry(this);
                resize(availableGeometry.width() / 2, availableGeometry.height() / 2);
                move((availableGeometry.width() - width()) / 2,
                     (availableGeometry.height() - height()) / 2);
            }

            restoreState(settings.value("windowstate").toByteArray());

            qDebug() << "After restore:" << this->pos() << " size:" << this->size();

            _musicCollectionDock->setVisible(false); /* because of restoreState above */
        }

        installEventFilter(this);
    }

    MainWindow::~MainWindow()
    {
        //
    }

    void MainWindow::createMenus()
    {
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

        _keepDisplayActiveAction =
                new QAction(tr("Keep &display active during playback"), this);
        _keepDisplayActiveAction->setCheckable(true);
        _keepDisplayActiveAction->setEnabled(_powerManagement->isPlatformSupported());
        connect(
            _keepDisplayActiveAction, &QAction::toggled,
            this, &MainWindow::updatePowerManagement
        );

        _aboutPmpAction = new QAction(tr("&About PMP..."), this);
        connect(
            _aboutPmpAction, &QAction::triggered, this, &MainWindow::onAboutPmpAction
        );

        _aboutQtAction = new QAction(tr("About &Qt..."), this);
        connect(_aboutQtAction, &QAction::triggered, this, &MainWindow::onAboutQtAction);

        /* Menus */

        QMenu* pmpMenu = menuBar()->addMenu(tr("&PMP"));

        pmpMenu->addAction(_startFullIndexationAction);

        QMenu* serverAdminMenu = pmpMenu->addMenu(tr("Server &administration"));
        _serverAdminAction = serverAdminMenu->menuAction();
        _serverAdminAction->setVisible(false); /* needs active connection */
        serverAdminMenu->addAction(_shutdownServerAction);

        pmpMenu->addSeparator();
        pmpMenu->addAction(_closeAction);

        _viewMenu = menuBar()->addMenu(tr("&View"));
        _viewMenu->menuAction()->setVisible(false); /* will be made visible after login */

        _viewMenu->addAction(_musicCollectionDock->toggleViewAction());
        _viewMenu->addSeparator();
        _viewMenu->addAction(_keepDisplayActiveAction);

        QMenu* helpMenu = menuBar()->addMenu(tr("&Help"));

        helpMenu->addAction(_aboutPmpAction);
        helpMenu->addAction(_aboutQtAction);
    }

    void MainWindow::createStatusbar()
    {
        _leftStatus = new QLabel("", this);
        _leftStatus->setFrameStyle(QFrame::Panel | QFrame::Sunken);
        _rightStatus = new QLabel("", this);
        _rightStatus->setFrameStyle(QFrame::Panel | QFrame::Sunken);

        statusBar()->addPermanentWidget(_leftStatus, 1);
        statusBar()->addPermanentWidget(_rightStatus, 1);

        connect(
            _leftStatusTimer, &QTimer::timeout, this, &MainWindow::onLeftStatusTimeout
        );

        updateRightStatus();
    }

    void MainWindow::closeEvent(QCloseEvent* event)
    {
        QSettings settings(QCoreApplication::organizationName(),
                           QCoreApplication::applicationName());

        settings.beginGroup("mainwindow");
        settings.setValue("geometry", saveGeometry());
        settings.setValue("windowstate", saveState());
        settings.setValue("musiccollectionvisible", _musicCollectionDock->isVisible());

        QMainWindow::closeEvent(event);
    }

    bool MainWindow::eventFilter(QObject* object, QEvent* event)
    {
        if (event->type() == QEvent::KeyPress)
        {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

            if (keyEventFilter(keyEvent))
                return true;
        }

        return QMainWindow::eventFilter(object, event);
    }

    bool MainWindow::keyEventFilter(QKeyEvent* event)
    {
        //qDebug() << "got key:" << event->key();

        /* we need an active connection for the actions of the multimedia buttons */
        if (!_connection) return false;

        switch (event->key())
        {
            case Qt::Key_MediaNext:
            {
                qDebug() << "got Next button";
                auto& controller = _clientServerInterface->playerController();

                if (controller.canSkip())
                    controller.skip();
                return true;
            }
            case Qt::Key_MediaPause:
            {
                qDebug() << "got Pause button";
                auto& controller = _clientServerInterface->playerController();

                if (controller.canPause())
                    controller.pause();
                return true;
            }
            case Qt::Key_MediaPlay:
            {
                qDebug() << "got Play button";
                auto& controller = _clientServerInterface->playerController();

                if (controller.canPlay())
                    controller.play();
                else if (controller.canPause())
                    controller.pause();
                return true;
            }
            case Qt::Key_MediaTogglePlayPause:
            {
                qDebug() << "got Play/Pause button";
                auto& controller = _clientServerInterface->playerController();

                if (controller.canPlay())
                    controller.play();
                else if (controller.canPause())
                    controller.pause();
                return true;
            }
        }

        return false;
    }

    void MainWindow::updateRightStatus()
    {
        if (!_connection || !_connection->isConnected())
        {
            _rightStatus->setText(tr("Not connected."));
        }
        else if (_connection->userLoggedInId() <= 0)
        {
            _rightStatus->setText(tr("Connected."));
        }
        else if (_connection->doingFullIndexation().toBool())
        {
            _rightStatus->setText(tr("Full indexation running..."));
        }
        else
        {
            _rightStatus->setText(
                QString(tr("Logged in as %1.")).arg(_connection->userLoggedInName())
            );
        }
    }

    void MainWindow::setLeftStatus(int intervalMs, QString text)
    {
        _leftStatus->setText(text);

        /* make the text disappear again after some time */
        _leftStatusTimer->stop();
        _leftStatusTimer->start(intervalMs);
    }

    void MainWindow::onLeftStatusTimeout()
    {
        _leftStatusTimer->stop();
        _leftStatus->setText("");
    }

    void MainWindow::onStartFullIndexationTriggered()
    {
        if (_connection) { _connection->startFullIndexation(); }
    }

    void MainWindow::onShutdownServerTriggered()
    {
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

        _clientServerInterface->generalController().shutdownServer();
    }

    void MainWindow::updatePowerManagement()
    {
        auto playerState =
                _clientServerInterface->playerController().playerState();
        bool isPlaying = playerState == PlayerState::Playing;

        bool keepDisplayActiveOption = _keepDisplayActiveAction->isChecked();

        _powerManagement->setKeepDisplayActive(isPlaying && keepDisplayActiveOption);
    }

    void MainWindow::onAboutPmpAction()
    {
        QString aboutText =
            tr(
                "<html>"
                "<h3>About PMP</h3>"
                "<p><b>Party Music Player</b>, abbreviated as PMP, is a client-server"
                " music system. The <i>server</i>, which is a separate program, plays the"
                " music. The program you are looking at right now, the <i>client</i>,"
                " is used as a remote control for the server. More than one client can"
                " connect to the same server, even at the same time.</p>"
                "<p>PMP is free and open-source software, using the GNU General Public "
                " License (GPLv3).</p>"
                "<p>Website: <a href=\"%1\">%1</a></p>"
                "<p>Report bugs at: <a href=\"%2\">%2</a></p>"
                "<p>Party Music Player <b>version %3</b><br>"
                "%4</p>" /* copyright line */
                "<p>Using Qt version %5</p>"
                "</html>"
            )
            .arg(PMP_WEBSITE)
            .arg(PMP_BUGREPORT_LOCATION)
            .arg(PMP_VERSION_DISPLAY)
            .arg(Util::getCopyrightLine(false))
            .arg(QT_VERSION_STR);

        QMessageBox::about(this, tr("About PMP"), aboutText);
    }

    void MainWindow::onAboutQtAction()
    {
        QMessageBox::aboutQt(this);
    }

    void MainWindow::onDoConnect(QString server, uint port)
    {
        _connection = new ServerConnection(this);
        _clientServerInterface = new ClientServerInterface(_connection);

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
            _connection, &ServerConnection::serverHealthChanged,
            this, &MainWindow::onServerHealthChanged
        );
        connect(
            _connection, &ServerConnection::fullIndexationStatusReceived,
            [this](bool running)
            {
                _startFullIndexationAction->setEnabled(
                    !running && _connection->isLoggedIn()
                );
                updateRightStatus();
            }
        );
        connect(
            _connection, &ServerConnection::fullIndexationStarted,
            [this]
            {
                setLeftStatus(3000, tr("Full indexation started"));
            }
        );
        connect(
            _connection, &ServerConnection::fullIndexationFinished,
            [this]
            {
                qDebug() << "fullIndexationFinished triggered";
                setLeftStatus(5000, tr("Full indexation finished"));
            }
        );
        connect(
            &_clientServerInterface->playerController(),
            &PlayerController::playerStateChanged,
            this, &MainWindow::updatePowerManagement
        );

        _connection->connectToHost(server, port);
    }

    void MainWindow::onConnected()
    {
        showUserAccountPicker();
        updateRightStatus();
    }

    void MainWindow::showUserAccountPicker()
    {
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

    void MainWindow::onCannotConnect(QAbstractSocket::SocketError error)
    {
        Q_UNUSED(error)

        QMessageBox::warning(
            this, tr("Connection failure"), tr("Failed to connect to that server.")
        );

        /* let the user try to correct any possible mistake */
        _connectionWidget->reenableFields();
    }

    void MainWindow::onInvalidServer()
    {
        QMessageBox::warning(
            this, tr("Connection failure"), tr("This is not a valid PMP server!")
        );

        /* let the user try to correct any possible mistake */
        _connectionWidget->reenableFields();
    }

    void MainWindow::onConnectionBroken(QAbstractSocket::SocketError error)
    {
        Q_UNUSED(error)

        updateRightStatus();

        QMessageBox::warning(
            this, tr("Connection failure"), tr("Connection to the server was lost!")
        );
        this->close();
    }

    void MainWindow::onServerHealthChanged(ServerHealthStatus serverHealth)
    {
        if (!serverHealth.anyProblems()) return;

        if (serverHealth.databaseUnavailable())
        {
            QMessageBox::warning(
                this, tr("Server problem"),
                tr("The server reports that its database is not working!")
            );
        }
        else
        {
            QMessageBox::warning(
                this, tr("Server problem"),
                tr("The server reports an unspecified problem!")
            );
        }
    }

    void MainWindow::showMainWidget()
    {
        _mainWidget = new MainWidget(this);
        _mainWidget->setConnection(_connection, _clientServerInterface);
        setCentralWidget(_mainWidget);

        auto collectionWidget =
                new CollectionWidget(_musicCollectionDock, _clientServerInterface);
        _musicCollectionDock->setWidget(collectionWidget);
        addDockWidget(Qt::RightDockWidgetArea, _musicCollectionDock);

        _viewMenu->menuAction()->setVisible(true);

        {
            QSettings settings(QCoreApplication::organizationName(),
                               QCoreApplication::applicationName());

            settings.beginGroup("mainwindow");
            _musicCollectionDock->setVisible(
                settings.value("musiccollectionvisible", true).toBool()
            );
        }
    }

    void MainWindow::onCreateAccountClicked()
    {
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
        Q_UNUSED(login)
        Q_UNUSED(password)
        Q_UNUSED(accountId)

        _userAccountCreationWidget = nullptr;
        showUserAccountPicker();
    }

    void MainWindow::onAccountCreationCancel()
    {
        _userAccountCreationWidget = nullptr;
        showUserAccountPicker();
    }

    void MainWindow::showLoginWidget(QString login)
    {
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

    void MainWindow::onLoggedIn(QString login)
    {
        Q_UNUSED(login)

        updateRightStatus();
        _connection->requestFullIndexationRunningStatus();

        _loginWidget = nullptr;
        showMainWidget();

        _startFullIndexationAction->setEnabled(false);
        _startFullIndexationAction->setVisible(true);
        _serverAdminAction->setVisible(true);
    }

    void MainWindow::onLoginCancel()
    {
        _loginWidget = nullptr;
        showUserAccountPicker();
    }
}
