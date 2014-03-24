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

    class FileData;
    class QueueEntry;

    class Player : public QObject {
        Q_OBJECT
    public:
        explicit Player(QObject* parent = 0);

        int volume() const;

        bool playing() const;

        QueueEntry const* nowPlaying() const;

    public slots:

        void playPause();
        void play();
        void pause();

        /** Skip the currently playing/paused track. Does nothing when not currently playing. */
        void skip();

        void setVolume(int volume);

        void clearQueue();
        void queue(QString const& filename);
        void queue(FileData const& filedata);

    Q_SIGNALS:

        void currentTrackChanged();
        void volumeChanged(int volume);

        /*! Emitted when the queue is empty and the current track is finished. */
        void finished();

    private slots:

        void internalStateChanged(QMediaPlayer::State state);
        bool startNext();

    private:
        QMediaPlayer* _player;
        QQueue<QueueEntry*> _queue;
        QueueEntry* _nowPlaying;
        bool _ignoreNextStopEvent;
    };
}
#endif
