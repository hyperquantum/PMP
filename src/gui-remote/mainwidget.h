/*
    Copyright (C) 2014-2020, Kevin Andre <hyperquantum@gmail.com>

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

#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QMenu)

namespace Ui {
    class MainWidget;
}

namespace PMP {

    class CurrentTrackMonitor;
    class PlayerHistoryModel;
    class QueueEntryInfoFetcher;
    class QueueMediator;
    class QueueModel;
    class QueueMonitor;
    class ServerConnection;
    class ServerInterface;
    class UserDataFetcher;

    class MainWidget : public QWidget {
        Q_OBJECT
    public:
        explicit MainWidget(QWidget *parent = 0);
        ~MainWidget();

        void setConnection(ServerConnection* connection,
                           ServerInterface* serverInterface);

    protected:
        bool eventFilter(QObject*, QEvent*);

    private slots:
        void playing(quint32 queueID);
        void paused(quint32 queueID);
        void stopped(quint32 queueLength);
        void queueLengthChanged(quint32 queueLength, int state);

        void trackProgress(quint32 queueID, quint64 position, int lengthSeconds);
        void trackProgress(quint64 position);
        void receivedTitleArtist(QString title, QString artist);
        void receivedPossibleFilename(QString name);

        void volumeChanged(int percentage);
        void decreaseVolume();
        void increaseVolume();

        void historyContextMenuRequested(const QPoint& position);
        void queueContextMenuRequested(const QPoint& position);

        void changeDynamicMode(int checkState);
        void noRepetitionIndexChanged(int index);
        void dynamicModeStatusReceived(bool enabled, int noRepetitionSpan);
        void dynamicModeHighScoreWaveStatusReceived(bool active, bool statusChanged);
        void startHighScoredTracksWave();

        void userPlayingForChanged(quint32 userId, QString login);

    private:
        bool keyEventFilter(QKeyEvent* event);

        void buildNoRepetitionList(int spanToSelect);
        QString noRepetitionTimeString(int seconds);

        Ui::MainWidget* _ui;
        ServerConnection* _connection;
        CurrentTrackMonitor* _currentTrackMonitor;
        QueueMonitor* _queueMonitor;
        QueueMediator* _queueMediator;
        QueueEntryInfoFetcher* _queueEntryInfoFetcher;
        QueueModel* _queueModel;
        QMenu* _queueContextMenu;
        int _volume;
        quint32 _nowPlayingQID;
        QString _nowPlayingTitle;
        QString _nowPlayingArtist;
        int _nowPlayingLength;
        bool _dynamicModeEnabled;
        bool _dynamicModeHighScoreWaveActive;
        QList<int> _noRepetitionList;
        int _noRepetitionUpdating;
        PlayerHistoryModel* _historyModel;
        QMenu* _historyContextMenu;
    };
}
#endif
