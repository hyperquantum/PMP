/*
    Copyright (C) 2014-2020, Kevin Andre <hyperquantum@gmail.com>

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

    PlayerInstance::PlayerInstance(QObject* parent, int identifier, Preloader* preloader,
                                   Resolver* resolver)
     : QObject(parent),
       _player(new QMediaPlayer(this)),
       _preloader(preloader),
       _resolver(resolver),
       _track(nullptr),
       _positionWhenStopped(-1),
       _identifier(identifier),
       _mediaSet(false),
       _endOfTrackComingUp(false),
       _hadSeek(false),
       _deleteAfterStopped(false)
    {
        connect(
            _player, &QMediaPlayer::mediaStatusChanged,
            this, &PlayerInstance::internalMediaStatusChanged
        );
        connect(
            _player, &QMediaPlayer::stateChanged,
            this, &PlayerInstance::internalStateChanged
        );
        connect(
            _player, &QMediaPlayer::positionChanged,
            this, &PlayerInstance::internalPositionChanged
        );
        connect(
            _player, &QMediaPlayer::durationChanged,
            this, &PlayerInstance::internalDurationChanged
        );
    }

    bool PlayerInstance::availableForNewTrack() const
    {
        return _player->state() != QMediaPlayer::PlayingState;
    }

    void PlayerInstance::setVolume(int volume)
    {
        qDebug() << "PlayerInstance" << _identifier
                 << " setvolume(" << volume << ") called";
        _player->setVolume(volume);
    }

    void PlayerInstance::setTrack(QueueEntry* queueEntry)
    {
        auto queueId = queueEntry->queueID();

        qDebug() << "PlayerInstance" << _identifier
                 << ": setTrack() called for queue ID" << queueId;

        if (_player->state() == QMediaPlayer::PlayingState) {
            qWarning() << "cannot set new track because we're still playing!";
            return;
        }

        _mediaSet = false;
        _endOfTrackComingUp = false;
        _hadSeek = false;
        _track = queueEntry;
        _positionWhenStopped = -1;

        if (!queueEntry || !queueEntry->isTrack())
            return;

        _preloadedFile = _preloader->getPreloadedCacheFile(queueId);
        QString filename = _preloadedFile.getFilename();

        if (filename.isEmpty()
                && queueEntry->checkValidFilename(*_resolver, true, &filename))
        {
            qWarning() << "QID" << queueId << "was not preloaded; "
                          "using unpreprocessed file that may be slow to access or "
                          "may fail to play";
        }

        if (!filename.isEmpty()) {
            qDebug() << "going to load media:" << filename;
            _player->setMedia(QUrl::fromLocalFile(filename));
            _mediaSet = true;
        }
    }

    void PlayerInstance::play()
    {
        qDebug() << "PlayerInstance" << _identifier << " play() called";
        _player->play();

        /* set start time if it hasn't been set yet */
        if (_track->started().isNull()) {
            _track->setStartedNow();
        }

        updateEndOfTrackComingUpFlag();
    }

    void PlayerInstance::pause()
    {
        qDebug() << "PlayerInstance" << _identifier << " pause() called";
        _player->pause();
    }

    void PlayerInstance::stop()
    {
        qDebug() << "PlayerInstance" << _identifier << " stop() called";
        _positionWhenStopped = _player->position();
        _player->stop();
    }

    void PlayerInstance::seekTo(qint64 position)
    {
        qDebug() << "PlayerInstance" << _identifier << " seek(" << position << ") called";

        _hadSeek = true;
        _player->setPosition(position);
        Q_EMIT positionChanged(position); /* notify all listeners immediately */

        updateEndOfTrackComingUpFlag();
    }

    void PlayerInstance::deleteAfterStopped()
    {
        _deleteAfterStopped = true;

        if (_player->state() == QMediaPlayer::StoppedState)
            this->deleteLater();
    }

    void PlayerInstance::internalStateChanged(QMediaPlayer::State state)
    {
        qDebug() << "PlayerInstance" << _identifier << ": state changed to" << state;

        switch (state) {
            case QMediaPlayer::StoppedState:
                switch (_player->mediaStatus()) {
                    case QMediaPlayer::EndOfMedia:
                        Q_EMIT trackFinished();
                        break;
                    case QMediaPlayer::InvalidMedia:
                        Q_EMIT playbackError();
                        break;
                    default:
                        Q_EMIT stoppedEarly(_positionWhenStopped);
                        break;
                }

                if (_deleteAfterStopped)
                    this->deleteLater();

                break;
            case QMediaPlayer::PausedState:
                Q_EMIT paused();
                break;
            case QMediaPlayer::PlayingState:
                Q_EMIT playing();
                break;
        }
    }

    void PlayerInstance::internalMediaStatusChanged(QMediaPlayer::MediaStatus status)
    {
        qDebug() << "PlayerInstance" << _identifier
                 << ": media state changed to" << status;
    }

    void PlayerInstance::internalPositionChanged(qint64 position)
    {
        Q_EMIT positionChanged(position);

        updateEndOfTrackComingUpFlag();
    }

    void PlayerInstance::internalDurationChanged(qint64 duration) {
        Q_UNUSED(duration)

        updateEndOfTrackComingUpFlag();
    }

    void PlayerInstance::updateEndOfTrackComingUpFlag() {
        auto lengthMilliseconds = _track->lengthInMilliseconds();
        if (lengthMilliseconds < 0) {
            qWarning() << "track duration not known, using backend duration";
            lengthMilliseconds = _player->duration();
            if (lengthMilliseconds <= 0) {
                qWarning() << "track duration still unknown";
                return;
            }
        }

        const int endOfTrackSpanMilliseconds = 5 * 1000; /* 5 seconds */

        auto positionMilliseconds = _player->position();

        bool endOfTrackComingUp =
                positionMilliseconds >= (lengthMilliseconds - endOfTrackSpanMilliseconds);

        if (endOfTrackComingUp != _endOfTrackComingUp) {
            _endOfTrackComingUp = endOfTrackComingUp;
            Q_EMIT endOfTrackComingUpChanged(endOfTrackComingUp);
        }
    }


    /* ================================================================================ */

    Player::Player(QObject* parent, Resolver* resolver, int defaultVolume)
     : QObject(parent),
       _oldInstance(nullptr),
       _currentInstance(nullptr),
       _nextInstance(nullptr),
       _resolver(resolver),
       //_player(new QMediaPlayer(this)),
       _queue(resolver), _preloader(nullptr, &_queue, resolver),
       _nowPlaying(nullptr),
       _instanceIdentifier(0),
       _volume(-1),
       _playPosition(0),
       //_maxPosReachedInCurrent(0),
       //_seekHappenedInCurrent(false),
       _state(ServerPlayerState::Stopped),
       //_transitioningToNextTrack(false),
       _userPlayingFor(0)
    {
        auto volume = (defaultVolume >= 0 && defaultVolume <= 100) ? defaultVolume : 75;
        setVolume(volume);
    }

    int Player::volume() const {
        return _volume;
    }

    bool Player::playing() const {
        return _state == ServerPlayerState::Playing;
    }

    ServerPlayerState Player::state() const {
        return _state;
    }

    void Player::changeState(ServerPlayerState state) {
        if (_state == state) return;

        qDebug() << "Player: state changed from" << int(_state) << "to" << int(state);
        _state = state;
        emit stateChanged(state);
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

    PlayerQueue& Player::queue() {
        return _queue;
    }

    Resolver& Player::resolver() {
        return *_resolver;
    }

    void Player::playPause() {
        if (_state == ServerPlayerState::Playing) {
            pause();
        }
        else {
            play();
        }
    }

    void Player::play() {
        switch (_state) {
            case ServerPlayerState::Stopped:
                startNext(false, true);
                break;
            case ServerPlayerState::Paused:
                if (_currentInstance) {
                    _currentInstance->play(); /* resume paused track */
                    changeState(ServerPlayerState::Playing);
                }
                break;
            case ServerPlayerState::Playing:
                break; /* already playing */
        }
    }

    void Player::pause() {
        switch (_state) {
            case ServerPlayerState::Stopped:
            case ServerPlayerState::Paused:
                break; /* no effect */
            case ServerPlayerState::Playing:
                if (_currentInstance) {
                    _currentInstance->pause();
                    changeState(ServerPlayerState::Paused);
                }
                break;
        }
    }

    void Player::skip() {
        bool mustPlay;

        switch (_state) {
            case ServerPlayerState::Stopped:
            default:
                return; /* do nothing */
            case ServerPlayerState::Paused:
                mustPlay = false;
                break;
            case ServerPlayerState::Playing:
                mustPlay = true;
                break;
        }

        /* start next track */
        startNext(true, mustPlay);
    }

    void Player::seekTo(qint64 position) {
        if (_state != ServerPlayerState::Playing && _state != ServerPlayerState::Paused)
            return;

        if (_currentInstance)
            _currentInstance->seekTo(position);
    }

    void Player::setVolume(int volume) {
        if (volume < 0 || volume > 100) return;
        if (volume == _volume) return;

        _volume = volume;

        if (_currentInstance)
            _currentInstance->setVolume(volume);

        Q_EMIT volumeChanged(volume);
    }

    void Player::setUserPlayingFor(quint32 user) {
        if (_userPlayingFor == user) return; /* no change */

        _userPlayingFor = user;
        emit userPlayingForChanged(user);
    }

    bool Player::startNext(bool stopCurrent, bool playNext) {
        qDebug() << "Player::startNext(" << stopCurrent << "," << playNext << ") called";

        /* move the previous old instance out of the way if necessary */
        if (_oldInstance && _currentInstance) {
            if (!_nextInstance && _oldInstance->availableForNewTrack()) {
                qDebug() << "moving previous old player instance to next instance";
                _nextInstance = _oldInstance;
            }
            else {
                qDebug() << "previous old player instance will be deleted";
                _oldInstance->deleteAfterStopped();
            }
            _oldInstance = nullptr;
        }

        QueueEntry* oldNowPlaying = _nowPlaying;
        uint oldQueueLength = _queue.length();

        /* current instance becomes old instance */
        if (_currentInstance) {
            _oldInstance = _currentInstance;
            _currentInstance = nullptr;
        }

        /* find next track to play and start it */
        QueueEntry* nextTrack = nullptr;
        while (!_queue.empty()) {
            QueueEntry* entry = _queue.dequeue();
            if (!entry->isTrack()) {
                if (entry->kind() == QueueEntryKind::Break) {
                    break; /* this will cause playback to stop because next == nullptr */
                }
                else { /* unknown non-track thing, let's just ignore it */
                    continue;
                }
            }

            putInHistoryOrder(entry);
            bool ok = tryStartNextTrack(entry, playNext);
            if (ok) {
                nextTrack = entry;
                break;
            }
            else {
                qWarning() << "failed to load/start track with queue ID"
                           << entry->queueID();

                addToHistory(entry, 0, true, false); /* register track as not played */
            }
        }

        if (stopCurrent && _oldInstance) {
            _oldInstance->stop();
        }

        ServerPlayerState newState;
        if (nextTrack) {
            newState = playNext ? ServerPlayerState::Playing : ServerPlayerState::Paused;

            /* try to figure out track length, and if possible tag, artist... */
            nextTrack->checkTrackData(*_resolver);
        }
        else {
            /* we stop because we have nothing left to play */
            newState = ServerPlayerState::Stopped;
        }

        _nowPlaying = nextTrack;

        if (!nextTrack)
            _playPosition = 0;

        if (oldNowPlaying || nextTrack)
            Q_EMIT currentTrackChanged(nextTrack);

        changeState(newState);

        if (!nextTrack && _queue.empty() && oldQueueLength > 0) {
            qDebug() << "queue is finished, nothing left to play";
            emit finished();
        }

        return nextTrack;
    }

    void Player::instancePlaying(PlayerInstance* instance) {
        if (instance != _currentInstance) return;

        changeState(ServerPlayerState::Playing);
    }

    void Player::instancePaused(PlayerInstance* instance) {
        if (instance != _currentInstance) return;

        changeState(ServerPlayerState::Paused);
    }

    void Player::instancePositionChanged(PlayerInstance* instance, qint64 position) {
        if (instance != _currentInstance) return;

        if (_state == ServerPlayerState::Stopped) {
            _playPosition = 0;
            return;
        }

        _playPosition = position;

        Q_EMIT positionChanged(position);
    }

    void Player::instanceEndOfTrackComingUpChanged(PlayerInstance* instance,
                                                   bool endOfTrackComingUp)
    {
        if (instance != _currentInstance) return;

        if (endOfTrackComingUp)
            prepareNextTrack();
    }

    void Player::instancePlaybackError(PlayerInstance* instance) {
        /* register track as not played */
        addToHistory(instance->track(), 0, true, false);
    }

    void Player::instanceTrackFinished(PlayerInstance* instance) {
        auto track = instance->track();
        bool hadSeek = instance->hadSeek();
        addToHistory(track, 1000, false, hadSeek);

        if (instance != _currentInstance) return;

        switch (_state) {
            case ServerPlayerState::Playing:
                startNext(false, true);
                break;
            case ServerPlayerState::Paused:
                startNext(false, false);
                break;
            case ServerPlayerState::Stopped:
                break;
        }
    }

    void Player::instanceStoppedEarly(PlayerInstance* instance, qint64 position) {
        auto track = instance->track();
        bool hadSeek = instance->hadSeek();
        auto permillage = calcPermillagePlayed(track, position, hadSeek);

        addToHistory(instance->track(), permillage, false, hadSeek);
    }

    PlayerInstance* Player::createNewPlayerInstance()
    {
        auto* instance =
                new PlayerInstance(this, ++_instanceIdentifier, &_preloader, _resolver);

        connect(
            instance, &PlayerInstance::playing,
            this, [=]() { this->instancePlaying(instance); }
        );
        connect(
            instance, &PlayerInstance::paused,
            this, [=]() { this->instancePaused(instance); }
        );
        connect(
            instance, &PlayerInstance::positionChanged,
            this,
            [=](qint64 position) { this->instancePositionChanged(instance, position); }
        );
        connect(
            instance, &PlayerInstance::endOfTrackComingUpChanged,
            this,
            [=](bool endOfTrackComingUp) {
                this->instanceEndOfTrackComingUpChanged(instance, endOfTrackComingUp);
            }
        );
        connect(
            instance, &PlayerInstance::playbackError,
            this, [=]() { this->instancePlaybackError(instance); }
        );
        connect(
            instance, &PlayerInstance::trackFinished,
            this, [=]() { this->instanceTrackFinished(instance); }
        );
        connect(
            instance, &PlayerInstance::stoppedEarly,
            this, [=](qint64 position) { this->instanceStoppedEarly(instance, position); }
        );

        return instance;
    }

    void Player::prepareNextTrack()
    {
        QueueEntry* nextTrack = _queue.peekFirstTrackEntry(4);
        if (!nextTrack) return; /* no next track to prepare for now */

        tryPrepareTrack(nextTrack); /* ignore the result */
    }

    bool Player::tryPrepareTrack(QueueEntry* entry)
    {
        if (!entry || !entry->isTrack())
            return false; /* argument should be a track */

        if (!_nextInstance) {
            if (_oldInstance && _oldInstance->availableForNewTrack()) {
                _nextInstance = _oldInstance;
                _oldInstance = nullptr;
            }
            else {
                _nextInstance = createNewPlayerInstance();
            }
        }
        else if (!_nextInstance->availableForNewTrack()) {
            qWarning() << "next instance not available for new track!";
            return false;
        }
        else if (_nextInstance->track() == entry) {
            return true; /* already prepared */
        }

        _nextInstance->setTrack(entry);
        return _nextInstance->trackSetSuccessfully();
    }

    bool Player::tryStartNextTrack(QueueEntry* entry, bool startPlaying)
    {
        if (!entry || !entry->isTrack())
            return false; /* argument should be a track */

        /* make sure we are prepared */
        bool prepared = tryPrepareTrack(entry);
        if (!prepared)
            return false; /* preparation went wrong */

        _currentInstance = _nextInstance;
        _nextInstance = nullptr;

        _playPosition = 0;

        _currentInstance->setVolume(_volume);

        if (startPlaying)
            _currentInstance->play();

        return true;
    }

    void Player::putInHistoryOrder(QueueEntry* entry)
    {
        _historyOrder.enqueue(entry->queueID());
    }

    void Player::addToHistory(QueueEntry* entry, int permillage, bool hadError,
                              bool hadSeek)
    {
        entry->setEndedNow(); /* register end time */

        if (!entry->isTrack())
            return; /* don't put breakpoints in the history */

        if (_historyOrder.isEmpty()) {
            qWarning()
                    << "history order is empty when adding a history entry for queue ID"
                    << entry->queueID();
        }

        if (hadSeek)
            permillage = -1;

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

        auto* hash = entry->hash();

        auto historyEntry =
            QSharedPointer<PlayerHistoryEntry>::create(
                queueID, hash ? *hash : FileHash(), userPlayedFor, started, ended,
                hadError, hadSeek, permillage
            );

        if (_historyOrder.isEmpty() || _historyOrder.first() != queueID) {
            _pendingHistory.insert(queueID, historyEntry);
            return;
        }

        _historyOrder.removeFirst();
        performHistoryActions(historyEntry);

        while (!_historyOrder.isEmpty()
               && _pendingHistory.contains(_historyOrder.first()))
        {
            auto entry = _pendingHistory.take(_historyOrder.dequeue());
            performHistoryActions(entry);
        }
    }

    void Player::performHistoryActions(QSharedPointer<PlayerHistoryEntry> historyEntry)
    {
        _queue.addToHistory(historyEntry);

        Q_EMIT newHistoryEntry(historyEntry);
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
