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

#ifndef PMP_MAINWIDGET_H
#define PMP_MAINWIDGET_H

#include <QWidget>

namespace Ui {
    class MainWidget;
}

namespace PMP {

    class ServerConnection;

    class MainWidget : public QWidget {
        Q_OBJECT

    public:
        explicit MainWidget(QWidget *parent = 0);
        ~MainWidget();

        void setConnection(ServerConnection* connection);

    private slots:
        void playing();
        void paused();
        void stopped();

        void volumeChanged(int percentage);
        void decreaseVolume();
        void increaseVolume();

        void noCurrentTrack();
        void nowPlayingTrack(quint32 queueID);
        void nowPlayingTrack(QString title, QString artist, int lengthInSeconds);
        void trackPositionChanged(quint64 position);
        void queueLengthChanged(int length);
        void receivedTrackInfo(quint32 queueID, int lengthInSeconds, QString title, QString artist);

    private:
        Ui::MainWidget* _ui;
        ServerConnection* _connection;
        int _volume;
        quint32 _nowPlayingQID;
        QString _nowPlayingTitle;
        QString _nowPlayingArtist;
        int _nowPlayingLength;
    };
}
#endif
