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

#ifndef PMP_PLAYER_H
#define PMP_PLAYER_H

#include <QMediaPlayer>
#include <QObject>
#include <QQueue>

namespace PMP {

    class Player : public QObject {
        Q_OBJECT

    public:
        Player(QObject* parent = 0);
        ~Player();

        int volume() const;

        bool playing() const;

    public slots:

        void playPause();
        void play();
        void pause();

        void setVolume(int volume);

        void clearQueue();
        void queue(QString filename);

    Q_SIGNALS:

        /*! Emitted when the queue is empty and the current track is finished. */
        void finished();

    private slots:

        void internalStateChanged(QMediaPlayer::State state);

    private:
        QMediaPlayer* _player;
        QQueue<QString> _queue;
    };
}
#endif
