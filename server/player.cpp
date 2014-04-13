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

#include "player.h"

#include "queueentry.h"
#include "resolver.h"

#include <QtDebug>

namespace PMP {

    Player::Player(QObject* parent, Resolver* resolver)
     : QObject(parent), _resolver(resolver),
        _player(new QMediaPlayer(this)),
        _nowPlaying(0),
        _ignoreNextStopEvent(false)
    {
        setVolume(75);
        connect(_player, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)), this, SLOT(internalMediaStatusChanged(QMediaPlayer::MediaStatus)));
        connect(_player, SIGNAL(stateChanged(QMediaPlayer::State)), this, SLOT(internalStateChanged(QMediaPlayer::State)));
        connect(_player, SIGNAL(volumeChanged(int)), this, SIGNAL(volumeChanged(int)));
    }

    int Player::volume() const {
        return _player->volume();
    }

    bool Player::playing() const {
        return _player->state() == QMediaPlayer::PlayingState;
    }

    QueueEntry const* Player::nowPlaying() const {
        return _nowPlaying;
    }

    void Player::playPause() {
        if (playing()) {
            pause();
        }
        else {
            play();
        }
    }

    void Player::play() {
        if (_player->state() == QMediaPlayer::PlayingState) {
            return; /* already playing */
        }

        if (_player->state() == QMediaPlayer::PausedState) {
            _player->play(); /* resume paused track */
            return;
        }

        /* we're not playing yet */
        startNext();
    }

    void Player::pause() {
        if (_player->state() == QMediaPlayer::PlayingState) {
            _player->pause();
        }
    }

    void Player::skip() {
        QMediaPlayer::State playerState = _player->state();
        if (playerState != QMediaPlayer::PlayingState
            && playerState != QMediaPlayer::PausedState)
        {
            return; /* skipping not possible in this state */
        }

        if (startNext()) {
            return; /* OK */
        }

        /* could not start the next track, so just stop */

        _player->stop(); /* further handling in internalStateChanged */
    }

    void Player::setVolume(int volume) {
        _player->setVolume(volume);
    }

    void Player::clearQueue() {
        _queue.clear();
    }

    void Player::queue(QString const& filename) {
        QueueEntry* entry = new QueueEntry(filename);

        _queue.enqueue(entry);
    }

    void Player::queue(FileData const& filedata) {
        QueueEntry* entry = new QueueEntry(filedata);

        _queue.enqueue(entry);
    }

    void Player::internalMediaStatusChanged(QMediaPlayer::MediaStatus state) {
        qDebug() << "Player::internalMediaStateChanged state:" << state;
    }

    void Player::internalStateChanged(QMediaPlayer::State state) {
        qDebug() << "Player::internalStateChanged state:" << state;
        if (state == QMediaPlayer::StoppedState) {
            if (_ignoreNextStopEvent) {
                _ignoreNextStopEvent = false;
            }
            else if (!startNext()) {
                /* stopped */
                _nowPlaying = 0;
                qDebug() << "stopped playing";
                emit currentTrackChanged(0);
                if (_queue.empty()) {
                    qDebug() << "finished queue";
                    emit finished();
                }
            }
        }
        else if (state == QMediaPlayer::PlayingState) {
            _ignoreNextStopEvent = false;
        }
    }

    bool Player::startNext() {
        qDebug() << "Player::startNext";

        /* TODO: save current track in history */

        while (!_queue.empty()) {
            QueueEntry* entry = _queue.dequeue();
            if (!entry->checkValidFilename()) {
                /* error */
                /* TODO: keep in history with failure note */
                delete entry;
                continue;
            }

            QString filename;
            if (entry->checkValidFilename(&filename)) {
                qDebug() << "loading media " << filename;
                _ignoreNextStopEvent = true; /* ignore the next stop event until we are playing again */
                _player->setMedia(QUrl::fromLocalFile(filename));
                _player->play();
                _nowPlaying = entry;
                emit currentTrackChanged(entry);
                return true;
            }
        }

        return false;
    }

}