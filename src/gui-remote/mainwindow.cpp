/*
    Copyright (C) 2014-2024, Kevin Andre <hyperquantum@gmail.com>

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

#include "common/powermanagement.h"
#include "common/unicodechars.h"
#include "common/util.h"
#include "common/version.h"

#include "client/generalcontroller.h"
#include "client/localhashidrepository.h"
#include "client/playercontroller.h"
#include "client/queuehashesmonitor.h"
#include "client/scrobblingcontroller.h"
#include "client/serverconnection.h"
#include "client/serverinterface.h"

#include "collectionwidget.h"
#include "connectionwidget.h"
#include "delayedstartdialog.h"
#include "delayedstartnotification.h"
#include "loginwidget.h"
#include "mainwidget.h"
#include "notificationbar.h"
#include "useraccountcreationwidget.h"
#include "userforstatisticsdisplay.h"
#include "userpickerwidget.h"
#include "userscrobblingdialog.h"

#include <QAction>
#include <QApplication>
#include <QCoreApplication>
#include <QDockWidget>
#include <QKeyEvent>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QScreen>
#include <QSettings>
#include <QStatusBar>
#include <QTimer>
#include <QVBoxLayout>

using namespace PMP::Client;

namespace PMP
{
    MainWindow::MainWindow(QWidget* parent)
     : QMainWindow(parent),
       _leftStatusTimer(new QTimer(this)),
       _connectionWidget(new ConnectionWidget(this)),
       _hashIdRepository(new LocalHashIdRepository()),
       _musicCollectionDock(new QDockWidget(tr("Music collection"), this)),
       _powerManagement(new PowerManagement(this))
    {
        setWindowTitle(
            QString(tr("Party Music Player ")) + UnicodeChars::enDash + tr(" Remote")
        );

        _musicCollectionDock->setObjectName("musicCollectionDockWidget");
        _musicCollectionDock->setAllowedAreas(
            (Qt::DockWidgetAreas)(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea)
        );

        createActions();
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

            auto geometryBeforeRestore = this->geometry();
            qDebug() << "Geometry before restore:" << geometryBeforeRestore;

            settings.beginGroup("mainwindow");
            restoreGeometry(settings.value("geometry").toByteArray());

            auto geometryAfterRestore = this->geometry();
            qDebug() << "Geometry after restore:" << geometryAfterRestore;

            if (geometryBeforeRestore == geometryAfterRestore)
                applyDefaultSizeAndPositionToWindow();
            else
                ensureWindowNotOffScreen(); // QTBUG-77385

            restoreState(settings.value("windowstate").toByteArray());

            _musicCollectionDock->setVisible(false); /* because of restoreState above */
        }

        installEventFilter(this);
    }

    MainWindow::~MainWindow()
    {
        delete _hashIdRepository;
    }

    void MainWindow::createActions()
    {
        _reloadServerSettingsAction = new QAction(tr("Re&load server settings"), this);
        connect(
            _reloadServerSettingsAction, &QAction::triggered,
            this, &MainWindow::onReloadServerSettingsTriggered
        );

        _shutdownServerAction = new QAction(tr("&Shutdown server"), this);
        connect(
            _shutdownServerAction, &QAction::triggered,
            this, &MainWindow::onShutdownServerTriggered
        );

        _scanForNewFilesAction = new QAction(tr("Scan for new files"), this);
        connect(
            _scanForNewFilesAction, &QAction::triggered,
            this, &MainWindow::onScanForNewFilesActionTriggered
        );

        _startFullIndexationAction = new QAction(tr("&Start full indexation"), this);
        connect(
            _startFullIndexationAction, &QAction::triggered,
            this, &MainWindow::onStartFullIndexationTriggered
        );

        _closeAction = new QAction(tr("&Close remote"), this);
        connect(_closeAction, &QAction::triggered, this, &MainWindow::close);

        _scrobblingAction = new QAction(tr("&Scrobbling..."));
        connect(
            _scrobblingAction, &QAction::triggered,
            this,
            [this]()
            {
                auto* dialog = new UserScrobblingDialog(this, _serverInterface);
                connect(dialog, &QDialog::finished, dialog, &QDialog::deleteLater);
                dialog->open();
            }
        );

        _activateDelayedStartAction = new QAction(tr("Activate &delayed start..."));
        connect(
            _activateDelayedStartAction, &QAction::triggered,
            this,
            [this]()
            {
                auto* dialog = new DelayedStartDialog(this, _serverInterface);
                connect(dialog, &QDialog::finished, dialog, &QDialog::deleteLater);
                dialog->open();
            }
        );

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
    }

    void MainWindow::createMenus()
    {
        /* Top-level menus */
        QMenu* pmpMenu = menuBar()->addMenu(tr("&PMP"));
        _userMenu = menuBar()->addMenu(tr("&User"));
        _actionsMenu = menuBar()->addMenu(tr("&Actions"));
        _viewMenu = menuBar()->addMenu(tr("&View"));
        QMenu* helpMenu = menuBar()->addMenu(tr("&Help"));

        /* "PMP" menu members */
        _indexationMenu = pmpMenu->addMenu(tr("&Indexation"));
        _serverAdminMenu = pmpMenu->addMenu(tr("Server &administration"));
        pmpMenu->addSeparator();
        pmpMenu->addAction(_closeAction);

        /* "PMP">"Indexation" menu members */
        _indexationMenu->addAction(_scanForNewFilesAction);
        _indexationMenu->addAction(_startFullIndexationAction);

        /* "PMP">"Server administration" menu members */
        _serverAdminMenu->addAction(_reloadServerSettingsAction);
        _serverAdminMenu->addSeparator();
        _serverAdminMenu->addAction(_shutdownServerAction);

        /* "User" menu members */
        _userMenu->addAction(_scrobblingAction);

        /* "Actions" menu members */
        _actionsMenu->addAction(_activateDelayedStartAction);

        /* "View" menu members */
        _viewMenu->addAction(_musicCollectionDock->toggleViewAction());
        _viewMenu->addSeparator();
        _viewMenu->addAction(_keepDisplayActiveAction);

        /* "Help" menu members */
        helpMenu->addAction(_aboutPmpAction);
        helpMenu->addAction(_aboutQtAction);

        /* Menu visibility */
        _indexationMenu->menuAction()->setVisible(false);
        _serverAdminMenu->menuAction()->setVisible(false); /* needs active connection */
        _userMenu->menuAction()->setVisible(false); /* will be made visible after login */
        _actionsMenu->menuAction()->setVisible(false);
        _viewMenu->menuAction()->setVisible(false); /* will be made visible after login */
    }

    void MainWindow::createStatusbar()
    {
        _leftStatus = new QLabel("", this);
        _leftStatus->setFrameStyle(QFrame::Panel | QFrame::Sunken);
        _rightStatus = new QLabel("", this);
        _rightStatus->setFrameStyle(QFrame::Panel | QFrame::Sunken);
        _scrobblingStatusLabel = new QLabel("", this);
        _scrobblingStatusLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
        _scrobblingStatusLabel->setVisible(false);

        statusBar()->addPermanentWidget(_leftStatus, 1);
        statusBar()->addPermanentWidget(_rightStatus, 1);
        statusBar()->addPermanentWidget(_scrobblingStatusLabel, 0);

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
        if (!_serverInterface || !_serverInterface->connected())
            return false;

        switch (event->key())
        {
            case Qt::Key_MediaNext:
            {
                qDebug() << "got Next button";
                auto& controller = _serverInterface->playerController();

                if (controller.canSkip())
                    controller.skip();
                return true;
            }
            case Qt::Key_MediaPause:
            {
                qDebug() << "got Pause button";
                auto& controller = _serverInterface->playerController();

                if (controller.canPause())
                    controller.pause();
                return true;
            }
            case Qt::Key_MediaPlay:
            {
                qDebug() << "got Play button";
                auto& controller = _serverInterface->playerController();

                if (controller.canPlay())
                    controller.play();
                else if (controller.canPause())
                    controller.pause();
                return true;
            }
            case Qt::Key_MediaTogglePlayPause:
            {
                qDebug() << "got Play/Pause button";
                auto& controller = _serverInterface->playerController();

                if (controller.canPlay())
                    controller.play();
                else if (controller.canPause())
                    controller.pause();
                return true;
            }
        }

        return false;
    }

    void MainWindow::enableDisableIndexationActions()
    {
        auto& generalController = _serverInterface->generalController();

        bool anyIndexationRunning =
            generalController.isFullIndexationRunning().toBool()
                || generalController.isQuickScanForNewFilesRunning().toBool();

        _scanForNewFilesAction->setEnabled(!anyIndexationRunning);
        _startFullIndexationAction->setEnabled(!anyIndexationRunning);
    }

    void MainWindow::updateRightStatus()
    {
        if (!_serverInterface || !_serverInterface->connected())
        {
            _rightStatus->setText(tr("Not connected."));
            return;
        }

        auto& generalController = _serverInterface->generalController();

        if (!_serverInterface->isLoggedIn())
        {
            _rightStatus->setText(tr("Connected."));
        }
        else if (generalController.isQuickScanForNewFilesRunning().toBool())
        {
            _rightStatus->setText(tr("Scanning for new files..."));
        }
        else if (generalController.isFullIndexationRunning().toBool())
        {
            _rightStatus->setText(tr("Full indexation running..."));
        }
        else
        {
            _rightStatus->setText(
                QString(tr("Logged in as %1.")).arg(_serverInterface->userLoggedInName())
            );
        }
    }

    void MainWindow::updateScrobblingUi()
    {
        auto& scrobblingController = _serverInterface->scrobblingController();

        auto lastFmEnabled = scrobblingController.lastFmEnabled();

        _scrobblingAction->setEnabled(lastFmEnabled != null);

        if (lastFmEnabled == false)
        {
            _scrobblingStatusLabel->setVisible(false);
            return;
        }

        auto lastFmStatus = scrobblingController.lastFmStatus();

        switch (lastFmStatus)
        {
        case ScrobblerStatus::Unknown:
            _scrobblingStatusLabel->setText(tr("Last.fm status: unknown"));
            break;
        case ScrobblerStatus::Green:
            _scrobblingStatusLabel->setText(tr("Last.fm status: good"));
            break;
        case ScrobblerStatus::Yellow:
            _scrobblingStatusLabel->setText(tr("Last.fm status: trying..."));
            break;
        case ScrobblerStatus::Red:
            _scrobblingStatusLabel->setText(tr("Last.fm status: BROKEN"));
            break;
        case ScrobblerStatus::WaitingForUserCredentials:
            _scrobblingStatusLabel->setText(tr("Last.fm status: NEED LOGIN"));
            break;
        default:
            qWarning() << "Scrobbler status not recognized:" << lastFmStatus;
            _scrobblingStatusLabel->setText(tr("Last.fm status: unknown"));
            break;
        }

        _scrobblingStatusLabel->setVisible(true);
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

    void MainWindow::applyDefaultSizeAndPositionToWindow()
    {
        auto* screen = QGuiApplication::primaryScreen();
        if (screen == nullptr)
        {
            qWarning() << "No primary screen found!";
            return;
        }

        auto availableGeometry = screen->availableGeometry();

        qDebug() << "Applying default position and size to main window";

        resize(availableGeometry.width() * 4 / 5,
               availableGeometry.height() * 4 / 5);

        move((availableGeometry.width() - width()) / 2 + availableGeometry.left(),
             (availableGeometry.height() - height()) / 2 + availableGeometry.top());
    }

    void MainWindow::ensureWindowNotOffScreen()
    {
        auto screen = QGuiApplication::screenAt(this->geometry().center());

        if (screen == nullptr
                || !screen->availableGeometry().contains(this->geometry().center()))
        {
            qDebug() << "main window appears to be off-screen (partially or completely)";
            applyDefaultSizeAndPositionToWindow();
        }
    }

    void MainWindow::onScanForNewFilesActionTriggered()
    {
        auto future = _serverInterface->generalController().startQuickScanForNewFiles();

        connectErrorPopupToActionResult(future,
                                        tr("Could not start a scan for new files."));
    }

    void MainWindow::onStartFullIndexationTriggered()
    {
        auto future = _serverInterface->generalController().startFullIndexation();

        connectErrorPopupToActionResult(future, tr("Could not start a full indexation."));
    }

    void MainWindow::onReloadServerSettingsTriggered()
    {
        auto future = _serverInterface->generalController().reloadServerSettings();

        future.addResultListener(
            this,
            [this](AnyResultMessageCode code)
            {
                reloadServerSettingsResultReceived(code);
            }
        );
    }

    void MainWindow::reloadServerSettingsResultReceived(AnyResultMessageCode errorCode)
    {
        QMessageBox msgBox;

        if (succeeded(errorCode))
        {
            msgBox.setIcon(QMessageBox::Information);
            msgBox.setText(tr("Server settings have been successfully reloaded."));
            msgBox.exec();
            return;
        }

        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText(tr("Server settings could not be reloaded."));

        if (errorCode == ResultMessageErrorCode::ServerTooOld)
        {
            msgBox.setInformativeText(
                tr("The server is too old and does not support reloading its settings.")
            );
        }
        else
        {
            msgBox.setInformativeText(
                tr("Error code: %1").arg(errorCodeString(errorCode))
            );
        }

        msgBox.exec();
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

        _serverInterface->generalController().shutdownServer();
    }

    void MainWindow::updatePowerManagement()
    {
        auto playerState = _serverInterface->playerController().playerState();
        bool isPlaying = playerState == PlayerState::Playing;

        bool keepDisplayActiveOption = _keepDisplayActiveAction->isChecked();

        _powerManagement->setKeepDisplayActive(isPlaying && keepDisplayActiveOption);
    }

    void MainWindow::onAboutPmpAction()
    {
        Nullable<VersionInfo> serverVersionInfo;
        QString serverVersionProblem;

        if (!_serverInterface || !_serverInterface->connected())
        {
            serverVersionProblem = tr("not connected to a server");
        }
        else
        {
            auto serverVersionFuture =
                _serverInterface->generalController().getServerVersionInfo();

            auto maybeServerVersionResultOrError =
                serverVersionFuture.resultOrErrorIfFinished();

            if (maybeServerVersionResultOrError == null
                || maybeServerVersionResultOrError.value().failed())
            {
                serverVersionProblem = tr("version unknown");
            }
            else
            {
                serverVersionInfo = maybeServerVersionResultOrError.value().result();
            }
        }

        const auto clientVersion = VersionInfo::current();
        const auto clientVersionText = getVersionText(clientVersion);

        const auto serverVersionText =
            (serverVersionProblem.isEmpty())
                ? getVersionText(serverVersionInfo.value())
                : tr("<i>%1</i>").arg(serverVersionProblem);

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
                "<hr>"
                "<p><b>Client</b>: %3<br>" /* program name, version, and possibly build info */
                "%4</p>" /* copyright line */
                "<p><b>Server</b>: %5</p>"
                "<p>Using Qt version %6</p>"
                "</html>"
            )
            .arg(PMP_WEBSITE,
                 PMP_BUGREPORT_LOCATION,
                 clientVersionText,
                 Util::getCopyrightLine(false),
                 serverVersionText,
                 QT_VERSION_STR);

        QMessageBox::about(this, tr("About PMP"), aboutText);
    }

    void MainWindow::onAboutQtAction()
    {
        QMessageBox::aboutQt(this);
    }

    void MainWindow::onDoConnect(QString server, uint port)
    {
        _connection = new ServerConnection(this, _hashIdRepository);
        _serverInterface = new ServerInterfaceImpl(_connection);

        auto* generalController = &_serverInterface->generalController();

        connect(
            _serverInterface, &ServerInterface::connectedChanged,
            this, &MainWindow::onConnectedChanged
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
            generalController, &GeneralController::serverHealthChanged,
            this, &MainWindow::onServerHealthChanged
        );
        connect(
            generalController, &GeneralController::fullIndexationStatusReceived,
            this,
            [this](StartStopEventStatus status)
            {
                enableDisableIndexationActions();
                updateRightStatus();

                if (Common::isChange(status))
                {
                    if (Common::isActive(status))
                    {
                        setLeftStatus(3000, tr("Full indexation started"));
                    }
                    else
                    {
                        setLeftStatus(5000, tr("Full indexation finished"));
                    }
                }
            }
        );
        connect(
            generalController, &GeneralController::quickScanForNewFilesStatusReceived,
            this,
            [this](StartStopEventStatus status)
            {
                enableDisableIndexationActions();
                updateRightStatus();

                if (Common::isChange(status))
                {
                    if (Common::isActive(status))
                    {
                        setLeftStatus(3000, tr("Scan for new files started"));
                    }
                    else
                    {
                        setLeftStatus(5000, tr("Scan for new files finished"));
                    }
                }
            }
        );
        connect(
            &_serverInterface->playerController(),
            &PlayerController::playerStateChanged,
            this, &MainWindow::updatePowerManagement
        );

        _connection->connectToHost(server, port);
    }

    void MainWindow::onConnectedChanged()
    {
        updateRightStatus();

        if (_serverInterface && _serverInterface->connected())
        {
            showUserAccountPicker();

            /* fetch server version */
            (void)_serverInterface->generalController().getServerVersionInfo();
        }
        else
        {
            QMessageBox::warning(
                this, tr("Connection failure"), tr("Connection to the server was lost!")
            );
            this->close();
        }
    }

    void MainWindow::showUserAccountPicker()
    {
        _userPickerWidget =
                new UserPickerWidget(this, &_serverInterface->generalController(),
                                     &_serverInterface->authenticationController());

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

    void MainWindow::onServerHealthChanged()
    {
        auto serverHealth = _serverInterface->generalController().serverHealth();

        if (!serverHealth.anyProblems())
            return;

        if (serverHealth.databaseUnavailable())
        {
            QMessageBox::warning(
                this, tr("Server problem"),
                tr("The server reports that its database is not working!")
            );
        }
        else if (serverHealth.sslLibrariesMissing())
        {
            QMessageBox::warning(
                this, tr("Server problem"),
                tr("The server reports that it does not have SSL libraries available! "
                   "This means that scrobbling will not work.")
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
        auto* mainCentralWidget = new QWidget(this);

        auto* delayedStartNotification =
            new DelayedStartNotification(this,
                                         &_serverInterface->playerController(),
                                         &_serverInterface->generalController());

        _notificationBar = new NotificationBar(mainCentralWidget);
        _notificationBar->addNotification(delayedStartNotification);

        auto* userForStatisticsDisplay =
            new UserForStatisticsDisplayImpl(this, _serverInterface);

        _mainWidget = new MainWidget(mainCentralWidget);
        _mainWidget->setConnection(_serverInterface, userForStatisticsDisplay);

        auto centralVerticalLayout = new QVBoxLayout(mainCentralWidget);
        centralVerticalLayout->setContentsMargins(0, 0, 0, 0);
        centralVerticalLayout->addWidget(_notificationBar);
        centralVerticalLayout->addWidget(_mainWidget);

        setCentralWidget(mainCentralWidget);

        auto queueHashesMonitor =
                new QueueHashesMonitorImpl(_serverInterface,
                                           &_serverInterface->queueMonitor(),
                                           &_serverInterface->queueEntryInfoStorage());

        auto collectionWidget =
                new CollectionWidget(_musicCollectionDock, _serverInterface,
                                     queueHashesMonitor, userForStatisticsDisplay);
        _musicCollectionDock->setWidget(collectionWidget);
        addDockWidget(Qt::RightDockWidgetArea, _musicCollectionDock);

        _actionsMenu->menuAction()->setVisible(true);
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
        auto* authenticationController = &_serverInterface->authenticationController();

        _userAccountCreationWidget =
            new UserAccountCreationWidget(this, authenticationController);

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
        _loginWidget =
                new LoginWidget(this,
                                &_serverInterface->authenticationController(),
                                login);

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

        _loginWidget = nullptr;
        showMainWidget();

        _indexationMenu->menuAction()->setVisible(true);
        _serverAdminMenu->menuAction()->setVisible(true);
        _userMenu->menuAction()->setVisible(true);

        auto* scrobblingController = &_serverInterface->scrobblingController();
        connect(
            scrobblingController, &ScrobblingController::lastFmInfoChanged,
            this, &MainWindow::updateScrobblingUi
        );

        updateScrobblingUi();
    }

    void MainWindow::onLoginCancel()
    {
        _loginWidget = nullptr;
        showUserAccountPicker();
    }

    void MainWindow::connectErrorPopupToActionResult(
                                                SimpleFuture<AnyResultMessageCode> future,
                                                QString failureText)
    {
        future.addResultListener(
            this,
            [failureText](AnyResultMessageCode code)
            {
                if (succeeded(code))
                    return; /* no need to display a message */

                QMessageBox msgBox;
                msgBox.setIcon(QMessageBox::Warning);
                msgBox.setText(failureText);

                if (code == ResultMessageErrorCode::ServerTooOld)
                {
                    msgBox.setInformativeText(
                        tr("The server is too old and does not support this action.")
                    );
                }
                else
                {
                    msgBox.setInformativeText(
                        tr("Error code: %1").arg(errorCodeString(code))
                    );
                }

                msgBox.exec();
            }
        );
    }

    QString MainWindow::getVersionText(const VersionInfo& versionInfo)
    {
        if (versionInfo.vcsBuild.isEmpty())
        {
            return tr("%1 <b>version %2</b>")
                .arg(versionInfo.programName,
                     versionInfo.versionForDisplay);
        }
        else
        {
            return tr("%1 <b>version %2</b> build %3 (%4)")
                .arg(versionInfo.programName,
                     versionInfo.versionForDisplay,
                     versionInfo.vcsBuild,
                     versionInfo.vcsBranch);
        }
    }
}
