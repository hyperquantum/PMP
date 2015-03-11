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

#ifndef PMP_MAINWIDGET_H
#define PMP_MAINWIDGET_H

#include <QWidget>

namespace Ui {
    class MainWidget;
}

namespace PMP {

    class CurrentTrackMonitor;
    class QueueEntryInfoFetcher;
    class QueueMediator;
    class QueueModel;
    class QueueMonitor;
    class ServerConnection;

    class MainWidget : public QWidget {
        Q_OBJECT

    public:
        explicit MainWidget(QWidget *parent = 0);
        ~MainWidget();

        void setConnection(ServerConnection* connection);

    protected:
        bool eventFilter(QObject*, QEvent*);

    private slots:
        void playing(quint32 queueID);
        void paused(quint32 queueID);
        void stopped();

        void trackProgress(quint32 queueID, quint64 position, int lengthSeconds);
        void trackProgress(quint64 position);
        void receivedTitleArtist(QString title, QString artist);
        void receivedPossibleFilename(QString name);

        void volumeChanged(int percentage);
        void decreaseVolume();
        void increaseVolume();

        void changeDynamicMode(int checkState);
        void noRepetitionIndexChanged(int index);
        void dynamicModeStatusReceived(bool enabled, int noRepetitionSpan);

        void queueLengthChanged(int length);

    private:
        void buildNoRepetitionList(int spanToSelect);
        QString noRepetitionTimeString(int seconds);

        Ui::MainWidget* _ui;
        ServerConnection* _connection;
        CurrentTrackMonitor* _currentTrackMonitor;
        QueueMonitor* _queueMonitor;
        QueueMediator* _queueMediator;
        QueueEntryInfoFetcher* _queueEntryInfoFetcher;
        QueueModel* _queueModel;
        int _volume;
        quint32 _nowPlayingQID;
        QString _nowPlayingTitle;
        QString _nowPlayingArtist;
        int _nowPlayingLength;
        bool _dynamicModeEnabled;
        QList<int> _noRepetitionList;
        int _noRepetitionUpdating;
    };
}
#endif
