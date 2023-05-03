/*
    Copyright (C) 2014-2023, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_MAINWIDGET_H
#define PMP_MAINWIDGET_H

#include "common/playerstate.h"

#include "client/localhashid.h"

#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QMenu)

namespace Ui
{
    class MainWidget;
}

namespace PMP::Client
{
    class QueueEntryInfoFetcher;
    class QueueMonitor;
    class ServerInterface;
    class UserDataFetcher;
}

namespace PMP
{
    class PlayerHistoryModel;
    class PreciseTrackProgressMonitor;
    class QueueMediator;
    class QueueModel;

    class MainWidget : public QWidget
    {
        Q_OBJECT
    public:
        explicit MainWidget(QWidget *parent = 0);
        ~MainWidget();

        void setConnection(Client::ServerInterface* serverInterface);

    protected:
        bool eventFilter(QObject*, QEvent*);

    private Q_SLOTS:
        void playerModeChanged();
        void playerStateChanged();
        void queueLengthChanged();
        void currentTrackChanged();
        void currentTrackInfoChanged();
        void trackProgressChanged(PlayerState state, quint32 queueId,
                                  qint64 progressInMilliseconds,
                                  qint64 trackLengthInMilliseconds);
        void switchTrackTimeDisplayMode();

        void trackInfoButtonClicked();
        void dynamicModeParametersButtonClicked();

        void volumeChanged();
        void decreaseVolume();
        void increaseVolume();

        void historyContextMenuRequested(const QPoint& position);
        void queueContextMenuRequested(const QPoint& position);

        void dynamicModeEnabledChanged();
        void changeDynamicMode(int checkState);

    private:
        void enableDisableTrackInfoButton();
        void enableDisablePlayerControlButtons();

        void updateTrackTimeDisplay();
        void updateTrackTimeDisplay(qint64 positionInMilliseconds,
                                    qint64 trackLengthInMilliseconds);

        void showTrackInfoDialog(Client::LocalHashId hashId, quint32 queueId = 0);

        bool keyEventFilter(QKeyEvent* event);

        Ui::MainWidget* _ui;
        Client::ServerInterface* _serverInterface;
        PreciseTrackProgressMonitor* _trackProgressMonitor;
        QueueMediator* _queueMediator;
        QueueModel* _queueModel;
        QMenu* _queueContextMenu;
        PlayerHistoryModel* _historyModel;
        QMenu* _historyContextMenu;
        bool _showingTimeRemaining;
    };
}
#endif
