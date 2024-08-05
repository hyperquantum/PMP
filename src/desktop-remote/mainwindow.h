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

#ifndef PMP_MAINWINDOW_H
#define PMP_MAINWINDOW_H

#include "common/newfuture.h"
#include "common/resultmessageerrorcode.h"

#include <QAbstractSocket>
#include <QMainWindow>

QT_FORWARD_DECLARE_CLASS(QAction)
QT_FORWARD_DECLARE_CLASS(QDockWidget)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QMenu)
QT_FORWARD_DECLARE_CLASS(QTimer)

namespace PMP::Client
{
    class LocalHashIdRepository;
    class ServerConnection;
    class ServerInterface;
}

namespace PMP
{
    class ConnectionWidget;
    class LoginWidget;
    class MainWidget;
    class NotificationBar;
    class PowerManagement;
    class UserAccountCreationWidget;
    class UserPickerWidget;
    struct VersionInfo;

    class MainWindow : public QMainWindow
    {
        Q_OBJECT

    public:
        MainWindow(QWidget* parent = 0);
        ~MainWindow();

    protected:
        bool eventFilter(QObject*, QEvent*);

    private Q_SLOTS:
        void onDoConnect(QString server, uint port);
        void onConnectedChanged();
        void onCannotConnect(QAbstractSocket::SocketError error);
        void onInvalidServer();
        void onServerHealthChanged();

        void onCreateAccountClicked();
        void onAccountCreated(QString login, QString password, quint32 accountId);
        void onAccountCreationCancel();

        void onLoggedIn(QString login);
        void onLoginCancel();

        void onScanForNewFilesActionTriggered();
        void onStartFullIndexationTriggered();
        void onReloadServerSettingsTriggered();
        void reloadServerSettingsResultReceived(AnyResultMessageCode errorCode);
        void onShutdownServerTriggered();
        void updatePowerManagement();
        void onAboutPmpAction();
        void onAboutQtAction();

        void onLeftStatusTimeout();

    private:
        void applyDefaultSizeAndPositionToWindow();
        void ensureWindowNotOffScreen();

        bool keyEventFilter(QKeyEvent* event);
        virtual void closeEvent(QCloseEvent* event);

        void createActions();
        void createMenus();
        void createStatusbar();
        void enableDisableIndexationActions();
        void updateRightStatus();
        void updateScrobblingUi();
        void setLeftStatus(int intervalMs, QString text);
        void showUserAccountPicker();
        void showLoginWidget(QString login);
        void showMainWidget();

        void connectErrorPopupToActionResult(NewSimpleFuture<AnyResultMessageCode> future,
                                             QString failureText);

        QString getVersionText(VersionInfo const& versionInfo);

        NotificationBar* _notificationBar { nullptr };
        QLabel* _leftStatus { nullptr };
        QLabel* _rightStatus { nullptr };
        QLabel* _scrobblingStatusLabel;
        QTimer* _leftStatusTimer;

        ConnectionWidget* _connectionWidget;
        Client::LocalHashIdRepository* _hashIdRepository;
        Client::ServerConnection* _connection { nullptr };
        Client::ServerInterface* _serverInterface { nullptr };
        UserPickerWidget* _userPickerWidget { nullptr };
        UserAccountCreationWidget* _userAccountCreationWidget { nullptr };
        LoginWidget* _loginWidget { nullptr };
        MainWidget* _mainWidget { nullptr };
        QDockWidget* _musicCollectionDock;

        QAction* _reloadServerSettingsAction;
        QAction* _shutdownServerAction;
        QAction* _scanForNewFilesAction;
        QAction* _startFullIndexationAction;
        QAction* _closeAction;
        QAction* _scrobblingAction;
        QAction* _activateDelayedStartAction;
        QAction* _keepDisplayActiveAction;
        QAction* _aboutPmpAction;
        QAction* _aboutQtAction;

        QMenu* _indexationMenu { nullptr };
        QMenu* _serverAdminMenu;
        QMenu* _userMenu;
        QMenu* _actionsMenu;
        QMenu* _viewMenu;

        PowerManagement* _powerManagement;
    };
}
#endif
