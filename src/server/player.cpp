/*
    Copyright (C) 2014-2019, Kevin Andre <hyperquantum@gmail.com>

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

    Player::Player(QObject* parent, Resolver* resolver, int defaultVolume)
     : QObject(parent), _resolver(resolver),
        _player(new QMediaPlayer(this)),
        _queue(resolver), _preloader(nullptr, &_queue, resolver),
        _nowPlaying(nullptr), _playPosition(0), _maxPosReachedInCurrent(0),
        _seekHappenedInCurrent(false),
        _state(PlayerState::Stopped), _transitioningToNextTrack(false),
        _userPlayingFor(0)
    {
        setVolume((defaultVolume >= 0 && defaultVolume <= 100) ? defaultVolume : 75);

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
        connect(
            _player, &QMediaPlayer::durationChanged,
            this, &Player::internalDurationChanged
        );
    }

    int Player::volume() const {
        return _player->volume();
    }

    bool Player::playing() const {
        return _state == PlayerState::Playing;
    }

    PlayerState Player::state() const {
        return _state;
    }

    void Player::changeStateTo(PlayerState state) {
        if (_state == state) return;

        qDebug() << "Player: state changed from" << int(_state) << "to" << int(state);
        _state = state;
        emit stateChanged(state);

        if (state == PlayerState::Playing) {
            emitStartedPlaying(_nowPlaying);
        }
    }

    QueueEntry const* Player::nowPlaying() const {
        return _nowPlaying;
    }

    uint Player::nowPlayingQID() const {
        QueueEntry* entry = _nowPlaying;
        return entry ? entry->queueID() : 0;
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
        if (_state == PlayerState::Playing) {
            pause();
        }
        else {
            play();
        }
    }

    void Player::play() {
        switch (_state) {
            case PlayerState::Stopped:
                startNext(true); /* we're not playing yet */
                break;
            case PlayerState::Paused:
                /* set start time if it hasn't been set yet */
                if (_nowPlaying->started().isNull()) {
                    _nowPlaying->setStartedNow();
                }
                _player->play(); /* resume paused track */
                changeStateTo(PlayerState::Playing);
                break;
            case PlayerState::Playing:
                break; /* already playing */
        }
    }

    void Player::pause() {
        switch (_state) {
            case PlayerState::Stopped:
            case PlayerState::Paused:
                break; /* no effect */
            case PlayerState::Playing:
                _player->pause();
                changeStateTo(PlayerState::Paused);
                break;
        }
    }

    void Player::skip() {
        bool mustPlay;

        switch (_state) {
            case PlayerState::Stopped:
            default:
                return; /* do nothing */
            case PlayerState::Paused:
                mustPlay = false;
                break;
            case PlayerState::Playing:
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
        if (_state != PlayerState::Playing && _state != PlayerState::Paused) return;
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
                    case PlayerState::Playing:
                        startNext(true);
                        break;
                    case PlayerState::Paused:
                        startNext(false);
                        break;
                    case PlayerState::Stopped:
                        break;
                }
            }
                break;
            case QMediaPlayer::PausedState:
                break; /* do nothing */
            case QMediaPlayer::PlayingState:
            {
                _transitioningToNextTrack = false;
            }
                break;
        }
    }

    void Player::internalPositionChanged(qint64 position) {
        if (_state == PlayerState::Stopped) { _playPosition = 0; return; }

        _playPosition = position;

        if (position > _maxPosReachedInCurrent)
            _maxPosReachedInCurrent = position;

        emit positionChanged(position);
    }

    void Player::internalDurationChanged(qint64 duration) {
        if (_nowPlaying == nullptr) return;

        auto previouslyKnownLength = _nowPlaying->lengthInMilliseconds();
        auto lengthFromPlayer = _player->duration();

        if (lengthFromPlayer <= 0) {
            qDebug() << "Player: don't know the actual track length yet";
        }
        else if (previouslyKnownLength != lengthFromPlayer
            && previouslyKnownLength > 0)
        {
            qDebug() << "Player: actual track length differs from known length;"
                     << AudioData::millisecondsToTimeString(previouslyKnownLength)
                     << "before,"
                     << AudioData::millisecondsToTimeString(lengthFromPlayer)
                     << "now";
        }
    }

    bool Player::startNext(bool play) {
        qDebug() << "Player::startNext";

        QueueEntry* oldNowPlaying = _nowPlaying;
        uint oldQueueLength = _queue.length();

        /* find next track to play */
        QueueEntry* next = nullptr;
        QString filename;
        while (!_queue.empty()) {
            QueueEntry* entry = _queue.dequeue();
            if (!entry->isTrack()) {
                if (entry->kind() == QueueEntryKind::Break) {
                    break; /* this will cause playback to stop because next == 0 */
                }
                continue;
            }

            filename = _preloader.getPreloadedCacheFileAndLock(entry->queueID());
            if (!filename.isEmpty()) {
                next = entry;
                break;
            }

            if (entry->checkValidFilename(*_resolver, true, &filename)) {
                next = entry;
                qWarning() << "QID" << entry->queueID()
                           << "was not preloaded; using unpreprocessed file that may be "
                              "slow to access or may fail to play";
                break;
            }

            /* error */
            qDebug() << "Player: skipping unplayable track (could not get filename)";
            addToHistory(entry, 0, true, false); /* register track as not played */
        }

        if (next) {
            _transitioningToNextTrack = true;
            _nowPlaying = next;
            _playPosition = 0;
            _maxPosReachedInCurrent = 0;
            _seekHappenedInCurrent = false;

            qDebug() << "Player: loading media " << filename;
            _player->setMedia(QUrl::fromLocalFile(filename));

            /* try to figure out track length, and if possible tag, artist... */
            next->checkTrackData(*_resolver);

            emit currentTrackChanged(next);

            if (play) {
                _nowPlaying->setStartedNow();
                if (_state != PlayerState::Playing) {
                    changeStateTo(PlayerState::Playing);
                }
                else {
                    emitStartedPlaying(_nowPlaying);
                }
                _player->play();
            }
            else {
                changeStateTo(PlayerState::Paused);
            }

            return true;
        }

        /* we stop because we have nothing left to play */

        _transitioningToNextTrack = true; /* to prevent duplicate history items */
        changeStateTo(PlayerState::Stopped);
        _player->stop();
        _preloader.lockNone();

        if (oldNowPlaying) {
            _nowPlaying = nullptr;
            _playPosition = 0;
            _maxPosReachedInCurrent = 0;
            _seekHappenedInCurrent = false;
            emit currentTrackChanged(nullptr);
        }

        if (_queue.empty() && oldQueueLength > 0) {
            qDebug() << "Player: queue is finished";
            emit finished();
        }

        return false;
    }

    void Player::emitStartedPlaying(QueueEntry const* queueEntry) {
        if (!queueEntry) return;

        auto startTime = queueEntry->started();
        int lengthInSeconds =
                static_cast<int>(queueEntry->lengthInMilliseconds() / 1000);

        emit startedPlaying(_userPlayingFor, startTime, queueEntry->title(),
                            queueEntry->artist(), queueEntry->album(), lengthInSeconds);
    }

    void Player::addToHistory(QueueEntry* entry, int permillage, bool hadError,
                              bool hadSeek)
    {
        entry->setEndedNow(); /* register end time */

        if (!entry->isTrack())
            return; /* don't put breakpoints in the history */

        auto userPlayedFor = _userPlayingFor;

        permillage = qMin(permillage, 1000); /* prevent overrun by wrong track lengths */

        auto queueID = entry->queueID();

        QDateTime started = entry->started();
        if (!started.isValid())
            started = QDateTime::currentDateTimeUtc();

        QDateTime ended = entry->ended();
        if (!ended.isValid()) {
            if (permillage > 0)
                ended = QDateTime::currentDateTimeUtc();
            else
                ended = started;
        }

        auto historyEntry =
            QSharedPointer<PlayerHistoryEntry>::create(
                queueID, userPlayedFor, started, ended, hadError, hadSeek, permillage
            );

        _queue.addToHistory(historyEntry);

        if (permillage <= 0 && hadError) {
            emit failedToPlayTrack(entry);
        }
        else {
            emit donePlayingTrack(entry, permillage, hadError, hadSeek);
        }

        emit trackHistoryEvent(
            queueID, started, ended, userPlayedFor, permillage, hadError, hadSeek
        );
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
        if (!track) return -2;

        auto msLength = track->lengthInMilliseconds();
        if (msLength < 0) return -3;

        return qBound(0, int(positionReached * 1000 / msLength), 1000);
    }

}
