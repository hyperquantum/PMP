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

#include "player.h"

#include "queueentry.h"
#include "resolver.h"

#include <QtDebug>
#include <QtGlobal>

namespace PMP {

    Player::Player(QObject* parent, Resolver* resolver)
     : QObject(parent), _resolver(resolver),
        _player(new QMediaPlayer(this)),
        _queue(resolver),
        _nowPlaying(0), _playPosition(0), _maxPosReachedInCurrent(0),
        _seekHappenedInCurrent(false),
        _state(Stopped), _transitioningToNextTrack(false),
        _userPlayingFor(0)
    {
        setVolume(75);

        connect(
            _player, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)),
            this, SLOT(internalMediaStatusChanged(QMediaPlayer::MediaStatus))
        );
        connect(
            _player, SIGNAL(stateChanged(QMediaPlayer::State)),
            this, SLOT(internalStateChanged(QMediaPlayer::State))
        );
        connect(
            _player, SIGNAL(positionChanged(qint64)),
            this, SLOT(internalPositionChanged(qint64))
        );
        connect(
            _player, SIGNAL(volumeChanged(int)),
            this, SIGNAL(volumeChanged(int))
        );
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

    uint Player::nowPlayingQID() const {
        QueueEntry* entry = _nowPlaying;
        return (entry == 0) ? 0 : entry->queueID();
    }

    qint64 Player::playPosition() const {
        return _playPosition;
    }

    quint32 Player::userPlayingFor() const {
        return _userPlayingFor;
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
                /* set start time if it hasn't been set yet */
                if (_nowPlaying->started().isNull()) {
                    _nowPlaying->setStartedNow();
                }
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
        bool mustPlay;

        switch (_state) {
            case Stopped:
            default:
                return; /* do nothing */
            case Paused:
                mustPlay = false;
                break;
            case Playing:
                mustPlay = true;
                break;
        }

        /* add previous track to history */
        if (_nowPlaying) {
            bool hadSeek = _seekHappenedInCurrent;
            addToHistory(_nowPlaying, calcPermillagePlayed(), false, hadSeek);
        }

        /* start next track */
        startNext(mustPlay);
    }

    void Player::seekTo(qint64 position) {
        if (_state != Playing && _state != Paused) return;
        //if (!_player->isSeekable()) return;

        _seekHappenedInCurrent = true;
        _player->setPosition(position);
        positionChanged(position); /* notify all listeners immediately */
    }

    void Player::setVolume(int volume) {
        _player->setVolume(volume);
    }

    void Player::setUserPlayingFor(quint32 user) {
        if (_userPlayingFor == user) return; /* no change */

        _userPlayingFor = user;
        emit userPlayingForChanged(user);
    }

    void Player::internalMediaStatusChanged(QMediaPlayer::MediaStatus state) {
        qDebug() << "Player::internalMediaStateChanged state:" << state;
    }

    void Player::internalStateChanged(QMediaPlayer::State state) {
        qDebug() << "Player::internalStateChanged state:" << state;

        switch (state) {
            case QMediaPlayer::StoppedState:
            {
                if (_transitioningToNextTrack) {
                    /* this is the stop event from the track we are
                       transitioning AWAY from, so we ignore it. */
                    _transitioningToNextTrack = false;
                    break;
                }

                /* add previous track to history */
                if (_nowPlaying) {
                    bool hadError = _player->mediaStatus() == QMediaPlayer::InvalidMedia;
                    bool hadSeek = _seekHappenedInCurrent;
                    addToHistory(_nowPlaying, calcPermillagePlayed(), hadError, hadSeek);
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
            }
                break;
            case QMediaPlayer::PausedState:
                break; /* do nothing */
            case QMediaPlayer::PlayingState:
                _transitioningToNextTrack = false;
                break;
        }
    }

    void Player::internalPositionChanged(qint64 position) {
        if (_state == Stopped) { _playPosition = 0; return; }

        _playPosition = position;

        if (position > _maxPosReachedInCurrent)
            _maxPosReachedInCurrent = position;

        emit positionChanged(position);
    }

    bool Player::startNext(bool play) {
        qDebug() << "Player::startNext";

        QueueEntry* oldNowPlaying = _nowPlaying;
        uint oldQueueLength = _queue.length();

        /* find next track to play */
        QueueEntry* next = 0;
        QString filename;
        while (!_queue.empty()) {
            QueueEntry* entry = _queue.dequeue();
            if (!entry->isTrack()) {
                if (entry->type() == QueueEntryType::Break) { play = false; }
                continue;
            }

            if (entry->checkValidFilename(*_resolver, true, &filename)) {
                next = entry;
                break;
            }

            /* error */
            qDebug() << " skipping unplayable track (could not get filename)";
            addToHistory(entry, 0, true, false); /* register track as not played */
        }

        if (next != 0) {
            _transitioningToNextTrack = true;
            qDebug() << " loading media " << filename;
            _player->setMedia(QUrl::fromLocalFile(filename));

            _nowPlaying = next;
            _playPosition = 0;
            _maxPosReachedInCurrent = 0;
            _seekHappenedInCurrent = false;

            /* try to figure out track length, and if possible tag, artist... */
            next->checkTrackData(*_resolver);

            emit currentTrackChanged(next);

            if (play) {
                _nowPlaying->setStartedNow();
                changeState(Playing);
                _player->play();
            }
            else {
                changeState(Paused);
            }

            return true;
        }

        /* we stop because we have nothing left to play */

        _transitioningToNextTrack = true; /* to prevent duplicate history items */
        changeState(Stopped);
        _player->stop();

        if (oldNowPlaying != 0 ) {
            _nowPlaying = 0;
            _playPosition = 0;
            _maxPosReachedInCurrent = 0;
            _seekHappenedInCurrent = false;
            emit currentTrackChanged(0);
        }

        if (_queue.empty() && oldQueueLength > 0) {
            qDebug() << "finished queue";
            emit finished();
        }

        return false;
    }

    void Player::addToHistory(QueueEntry *entry, int permillage, bool hadError,
                              bool hadSeek)
    {
        _nowPlaying->setEndedNow(); /* register end time */

        permillage = qMin(permillage, 1000); /* prevent overrun by wrong track lengths */

        _queue.addToHistory(entry, permillage, hadError);

        if (permillage <= 0 && hadError) {
            emit failedToPlayTrack(entry);
        }
        else {
            emit donePlayingTrack(entry, permillage, hadError, hadSeek);
        }
    }

    int Player::calcPermillagePlayed() {
        qint64 position = _player->position();

        if (position > _maxPosReachedInCurrent) {
            qDebug() << "adjusting maximum position reached from"
                     << _maxPosReachedInCurrent << "to" << position;
            _maxPosReachedInCurrent = position;
        }

        return calcPermillagePlayed(
            _nowPlaying, _maxPosReachedInCurrent, _seekHappenedInCurrent
        );
    }

    /* permillage (for lack of a better name) is like percentage, but with factor 1000
       instead of 100 */
    int Player::calcPermillagePlayed(QueueEntry* track, qint64 positionReached,
                                     bool seeked)
    {
        if (seeked) return -1;
        if (track == 0) return -2;

        int secsLength = track->lengthInSeconds();
        if (secsLength < 0) return -3;

        /* positionReached is in milliseconds (seconds times 1000), and length is in
           seconds, so we do not need to multiply with 1000 again */
        return qBound(0, int(positionReached / secsLength), 1000);
    }

}
