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

#include "queue.h"

#include <QMediaPlayer>

namespace PMP {

    class Resolver;

    class Player : public QObject {
        Q_OBJECT
    public:
        enum State { Stopped, Playing, Paused };

        Player(QObject* parent, Resolver* resolver);

        int volume() const;

        bool playing() const;
        State state() const;
        QueueEntry const* nowPlaying() const;
        uint nowPlayingQID() const;
        qint64 playPosition() const;

        Queue& queue();

        Resolver& resolver();

    public slots:

        void playPause();
        void play();
        void pause();

        /** Skip the currently playing/paused track. Does nothing when not currently playing. */
        void skip();

        void seekTo(qint64 position);

        void setVolume(int volume);

    Q_SIGNALS:

        void stateChanged(Player::State state);
        void currentTrackChanged(QueueEntry const* newTrack);
        void positionChanged(qint64 position);
        void volumeChanged(int volume);

        /*! Emitted when the queue is empty and the current track is finished. */
        void finished();

    private slots:
        void changeState(State state);
        void internalStateChanged(QMediaPlayer::State state);
        void internalMediaStatusChanged(QMediaPlayer::MediaStatus);
        void internalPositionChanged(qint64 position);
        bool startNext(bool play);

    private:
        int calcPermillagePlayed();
        static int calcPermillagePlayed(QueueEntry* track, qint64 positionReached, bool seeked);

        Resolver* _resolver;
        QMediaPlayer* _player;
        Queue _queue;
        QueueEntry* _nowPlaying;
        qint64 _playPosition;
        qint64 _maxPosReachedInCurrent;
        bool _seekHappenedInCurrent;
        State _state;
        bool _transitioningToNextTrack;
    };
}
#endif
