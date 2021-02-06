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

#ifndef PMP_MAINWIDGET_H
#define PMP_MAINWIDGET_H

#include "common/filehash.h"
#include "common/playerstate.h"

#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QMenu)

namespace Ui {
    class MainWidget;
}

namespace PMP {

    class ClientServerInterface;
    class PlayerHistoryModel;
    class PreciseTrackProgressMonitor;
    class QueueEntryInfoFetcher;
    class QueueMediator;
    class QueueModel;
    class QueueMonitor;
    class ServerConnection;
    class UserDataFetcher;

    class MainWidget : public QWidget {
        Q_OBJECT
    public:
        explicit MainWidget(QWidget *parent = 0);
        ~MainWidget();

        void setConnection(ServerConnection* connection,
                           ClientServerInterface* clientServerInterface);

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

        void trackInfoButtonClicked();

        void volumeChanged();
        void decreaseVolume();
        void increaseVolume();

        void historyContextMenuRequested(const QPoint& position);
        void queueContextMenuRequested(const QPoint& position);

        void dynamicModeEnabledChanged();
        void noRepetitionSpanSecondsChanged();
        void changeDynamicMode(int checkState);
        void noRepetitionIndexChanged(int index);

        void waveActiveChanged();
        void waveProgressChanged();
        void startHighScoredTracksWave();
        void terminateHighScoredTracksWave();

    private:
        void enableDisableTrackInfoButton();
        void enableDisablePlayerControlButtons();

        void showTrackInfoDialog(FileHash hash, quint32 queueId = 0);

        bool keyEventFilter(QKeyEvent* event);

        void buildNoRepetitionList(int spanToSelect);
        QString noRepetitionTimeString(int seconds);

        Ui::MainWidget* _ui;
        ClientServerInterface* _clientServerInterface;
        PreciseTrackProgressMonitor* _trackProgressMonitor;
        QueueMediator* _queueMediator;
        QueueModel* _queueModel;
        QMenu* _queueContextMenu;
        QList<int> _noRepetitionList;
        int _noRepetitionUpdating;
        PlayerHistoryModel* _historyModel;
        QMenu* _historyContextMenu;
    };
}
#endif
