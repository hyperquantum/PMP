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

namespace PMP {

    Player::Player(QObject* parent)
     : QObject(parent),
        _player(new QMediaPlayer(this))
    {
        setVolume(75);
        connect(_player, SIGNAL(stateChanged(QMediaPlayer::State)), this, SLOT(internalStateChanged(QMediaPlayer::State)));
        connect(_player, SIGNAL(volumeChanged(int)), this, SIGNAL(volumeChanged(int)));
    }

    int Player::volume() const {
        return _player->volume();
    }

    bool Player::playing() const {
        return _player->state() == QMediaPlayer::PlayingState;
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
            return; // already playing
        }

        if (_player->state() == QMediaPlayer::PausedState) {
            _player->play();
            return;
        }

        // stopped

        if (_queue.empty()) {
            return; // nothing to play
        }

        QString file = _queue.dequeue();
        _player->setMedia(QUrl::fromLocalFile(file));
        _player->play();
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
            return; // not playing
        }

        if (_queue.empty()) {
            // no next track
            _player->stop(); // further handling in internalStateChanged
            return;
        }

        QString file = _queue.dequeue();
        _player->setMedia(QUrl::fromLocalFile(file));
        _player->play();
    }

    void Player::setVolume(int volume) {
        _player->setVolume(volume);
    }

    void Player::clearQueue() {
        _queue.clear();
    }

    void Player::queue(QString filename) {
        _queue.enqueue(filename);
    }

    void Player::internalStateChanged(QMediaPlayer::State state) {
        if (state == QMediaPlayer::StoppedState) {
            if (_queue.empty()) {
                // stopped
                emit finished();
            }
            else {
                QString file = _queue.dequeue();
                _player->setMedia(QUrl::fromLocalFile(file));
                _player->play();
            }
        }
    }

}
