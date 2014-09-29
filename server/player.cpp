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
        _nowPlaying(0), _playPosition(0),
        _state(Stopped), _ignoreNextStopEvent(false)
    {
        setVolume(75);
        connect(_player, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)), this, SLOT(internalMediaStatusChanged(QMediaPlayer::MediaStatus)));
        connect(_player, SIGNAL(stateChanged(QMediaPlayer::State)), this, SLOT(internalStateChanged(QMediaPlayer::State)));
        connect(_player, SIGNAL(positionChanged(qint64)), this, SLOT(internalPositionChanged(qint64)));
        connect(_player, SIGNAL(volumeChanged(int)), this, SIGNAL(volumeChanged(int)));
    }

    int Player::volume() const {
        return _player->volume();
    }

    bool Player::playing() const {
        return _state == Playing;
    }

    Player::State Player::state() const {
        return _state;
    }

    void Player::changeState(State state) {
        if (_state == state) return;

        qDebug() << "state changed from" << _state << "to" << state;
        _state = state;
        emit stateChanged(state);
    }

    QueueEntry const* Player::nowPlaying() const {
        return _nowPlaying;
    }

    qint64 Player::playPosition() const {
        return _playPosition;
    }

    Queue& Player::queue() {
        return _queue;
    }

    Resolver& Player::resolver() {
        return *_resolver;
    }

    void Player::playPause() {
        if (_state == Playing) {
            pause();
        }
        else {
            play();
        }
    }

    void Player::play() {
        switch (_state) {
            case Stopped:
                startNext(true); /* we're not playing yet */
                break;
            case Paused:
                _player->play(); /* resume paused track */
                changeState(Playing);
                break;
            case Playing:
                break; /* already playing */
        }
    }

    void Player::pause() {
        switch (_state) {
            case Stopped:
            case Paused:
                break; /* no effect */
            case Playing:
                _player->pause();
                changeState(Paused);
                break;
        }
    }

    void Player::skip() {
        switch (_state) {
            case Stopped:
                break; /* no effect */
            case Paused:
                startNext(false);
                break;
            case Playing:
                startNext(true);
                break;
        }
    }

    void Player::setVolume(int volume) {
        _player->setVolume(volume);
    }

    void Player::internalMediaStatusChanged(QMediaPlayer::MediaStatus state) {
        qDebug() << "Player::internalMediaStateChanged state:" << state;
    }

    void Player::internalStateChanged(QMediaPlayer::State state) {
        qDebug() << "Player::internalStateChanged state:" << state;

        switch (state) {
            case QMediaPlayer::StoppedState:
                if (_ignoreNextStopEvent) {
                    _ignoreNextStopEvent = false;
                    break;
                }

                switch (_state) {
                    case Playing:
                        startNext(true);
                        break;
                    case Paused:
                        startNext(false);
                        break;
                    case Stopped:
                        break;
                }

                break;
            case QMediaPlayer::PausedState:
                break; /* do nothing */
            case QMediaPlayer::PlayingState:
                _ignoreNextStopEvent = false;
                break;
        }
    }

    void Player::internalPositionChanged(qint64 position) {
        if (_state == Stopped) { _playPosition = 0; return; }

        _playPosition = position;
        emit positionChanged(position);
    }

    bool Player::startNext(bool play) {
        qDebug() << "Player::startNext";

        //State initialState = _state;
        QueueEntry* oldNowPlaying = _nowPlaying;
        uint oldQueueLength = _queue.length();

        /* TODO: save current track in history */

        while (!_queue.empty()) {
            QueueEntry* entry = _queue.dequeue();
            QString filename;
            if (!entry->checkValidFilename(*_resolver, &filename)) {
                /* error */
                qDebug() << " skipping unplayable track (could not get filename)";
                /* TODO: keep in history with failure note */
                delete entry;
                continue;
            }

            qDebug() << " loading media " << filename;
            _ignoreNextStopEvent = true; /* ignore the next stop event until we are playing again */
            _player->setMedia(QUrl::fromLocalFile(filename));
            _nowPlaying = entry;
            _playPosition = 0;
            entry->checkTrackData(*_resolver);
            emit currentTrackChanged(entry);

            if (play) {
                changeState(Playing);
                _player->play();
            }

            /* TODO: move elsewhere */
            int tracksToGenerate = 1;
            if (_queue.length() < 10) { tracksToGenerate = 10 - _queue.length(); }
            while (tracksToGenerate > 0) {
                tracksToGenerate--;
                HashID randomHash = _resolver->getRandom();
                if (randomHash.empty()) { break; /* nothing available */ }
                _queue.enqueue(randomHash);
            }

            return true;
        }

        /* stopped */

        changeState(Stopped);
        _player->stop();

        if (oldNowPlaying != 0 ) {
            _nowPlaying = 0;
            _playPosition = 0;
            emit currentTrackChanged(0);
        }

        if (_queue.empty() && oldQueueLength > 0) {
            qDebug() << "finished queue";
            emit finished();
        }

        return false;
    }

}
