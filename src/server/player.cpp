/*
    Copyright (C) 2014-2024, Kevin Andre <hyperquantum@gmail.com>

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

#include <QAudio>
#include <QAudioOutput>
#include <QMediaPlayer>
#include <QtDebug>
#include <QtGlobal>

namespace PMP::Server
{
    PlayerInstance::PlayerInstance(QObject* parent, int identifier, Preloader* preloader,
                                   Resolver* resolver)
     : QObject(parent),
       _audioOutput(new QAudioOutput()),
       _player(new QMediaPlayer(this)),
       _preloader(preloader),
       _resolver(resolver),
       _track(nullptr),
       _positionWhenStopped(-1),
       _identifier(identifier),
       _availableForNewTrack(true),
       _mediaSet(false),
       _endOfTrackComingUp(false),
       _hadSeek(false),
       _deleteAfterStopped(false)
    {
        _player->setAudioOutput(_audioOutput);

        connect(
            _player, &QMediaPlayer::errorChanged,
            this, &PlayerInstance::internalErrorChanged
        );
        connect(
            _player, &QMediaPlayer::mediaStatusChanged,
            this, &PlayerInstance::internalMediaStatusChanged
        );
        connect(
            _player, &QMediaPlayer::playbackStateChanged,
            this, &PlayerInstance::internalPlaybackStateChanged
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
        return _availableForNewTrack;
    }

    qint64 PlayerInstance::position() const
    {
        if (!_mediaSet) return 0;
        return _player->position();
    }

    void PlayerInstance::setVolume(int volume)
    {
        qDebug() << "PlayerInstance" << _identifier
                 << ": setting volume to" << volume;

        auto linearVolume =
            QAudio::convertVolume(volume / float(100),
                                  QAudio::LogarithmicVolumeScale,
                                  QAudio::LinearVolumeScale);

        _audioOutput->setVolume(linearVolume);
    }

    void PlayerInstance::setTrack(QSharedPointer<QueueEntry> queueEntry,
                                  bool onlyIfPreloaded)
    {
        auto queueId = queueEntry->queueID();

        qDebug() << "PlayerInstance" << _identifier
                 << ": setTrack() called for queue ID" << queueId;

        if (!availableForNewTrack())
        {
            qWarning() << "PlayerInstance" << _identifier
                       << "cannot set new track because the instance is not available!";
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

        if (filename.isEmpty() && onlyIfPreloaded)
        {
            qDebug() << "PlayerInstance: queue ID" << queueId
                     << "not preloaded yet, will not set media now";
            return;
        }

        if (filename.isEmpty())
        {
            filename = queueEntry->filename().valueOr({});

            if (!filename.isEmpty())
            {
                if (_resolver->pathStillValid(queueEntry->hash().value(), filename))
                {
                    qWarning() << "queue ID" << queueId << "was not preloaded; "
                                  "will try to use unpreprocessed file that may be slow "
                                  "to access or may fail to play";
                }
                else
                {
                    filename.clear();
                    queueEntry->invalidateFilename();
                }
            }
        }

        if (!filename.isEmpty())
        {
            qDebug() << "PlayerInstance" << _identifier << "for queue ID" << queueId
                     << ": going to load media:" << filename;
            _player->setSource(QUrl::fromLocalFile(filename));
            _mediaSet = true;
        }
    }

    void PlayerInstance::play()
    {
        qDebug() << "PlayerInstance" << _identifier << " play() called";
        _availableForNewTrack = false;
        _player->play();

        /* set start time if it hasn't been set yet */
        if (_track->started().isNull())
        {
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

        if (_player->playbackState() == QMediaPlayer::StoppedState)
            this->deleteLater();
    }

    void PlayerInstance::internalPlaybackStateChanged()
    {
        qDebug() << "PlayerInstance" << _identifier
                 << ": playback state changed to" << _player->playbackState();

        switch (_player->playbackState())
        {
            case QMediaPlayer::StoppedState:
                switch (_player->mediaStatus())
                {
                    case QMediaPlayer::EndOfMedia:
                        Q_EMIT trackFinished();
                        break;
                    case QMediaPlayer::InvalidMedia:
                        qDebug() << "'stopped' state combined with 'invalid media'";
                        Q_EMIT playbackError();
                        break;
                    default:
                        Q_EMIT stoppedEarly(_positionWhenStopped);
                        break;
                }

                _availableForNewTrack = true;

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

    void PlayerInstance::internalMediaStatusChanged()
    {
        qDebug() << "PlayerInstance" << _identifier
                 << ": media status changed to" << _player->mediaStatus();
    }

    void PlayerInstance::internalErrorChanged()
    {
        qDebug() << "PlayerInstance" << _identifier
                 << ": error changed to" << _player->error()
                 << "with error string:" << _player->errorString();
    }

    void PlayerInstance::internalPositionChanged(qint64 position)
    {
        Q_EMIT positionChanged(position);

        updateEndOfTrackComingUpFlag();
    }

    void PlayerInstance::internalDurationChanged(qint64 duration)
    {
        Q_UNUSED(duration)

        updateEndOfTrackComingUpFlag();
    }

    void PlayerInstance::updateEndOfTrackComingUpFlag()
    {
        auto lengthMilliseconds = _track->lengthInMilliseconds();
        if (lengthMilliseconds < 0)
        {
            qWarning() << "PlayerInstance" << _identifier
                       << ": track duration not known, using backend duration";
            lengthMilliseconds = _player->duration();
            if (lengthMilliseconds <= 0)
            {
                qWarning() << "PlayerInstance" << _identifier
                           << ": track duration still unknown";
                return;
            }
        }

        const int endOfTrackSpanMilliseconds = 5 * 1000; /* 5 seconds */

        auto positionMilliseconds = _player->position();

        bool endOfTrackComingUp =
                positionMilliseconds >= (lengthMilliseconds - endOfTrackSpanMilliseconds);

        if (endOfTrackComingUp != _endOfTrackComingUp)
        {
            _endOfTrackComingUp = endOfTrackComingUp;
            Q_EMIT endOfTrackComingUpChanged(endOfTrackComingUp);
        }
    }


    /* ================================================================================ */

    Player::Player(QObject* parent, Resolver* resolver, int defaultVolume)
     : QObject(parent),
       _oldInstance1(nullptr),
       _oldInstance2(nullptr),
       _currentInstance(nullptr),
       _nextInstance(nullptr),
       _resolver(resolver),
       _queue(resolver), _preloader(nullptr, &_queue, resolver),
       _nowPlaying(nullptr),
       _instanceIdentifier(0),
       _volume(-1),
       _playPosition(0),
       _state(ServerPlayerState::Stopped),
       _userPlayingFor(0)
    {
        auto volume = (defaultVolume >= 0 && defaultVolume <= 100) ? defaultVolume : 75;
        setVolume(volume);

        connect(
            &_queue, &PlayerQueue::firstTrackChanged,
            this, &Player::firstTrackInQueueChanged
        );
        connect(
            &_preloader, &Preloader::trackPreloaded,
            this, &Player::trackPreloaded
        );

        /* speed up things by creating the player instances at startup */
        _nextInstance = createNewPlayerInstance();
        _oldInstance1 = createNewPlayerInstance();
        _oldInstance2 = createNewPlayerInstance();
    }

    int Player::volume() const
    {
        return _volume;
    }

    bool Player::playing() const
    {
        return _state == ServerPlayerState::Playing;
    }

    ServerPlayerState Player::state() const
    {
        return _state;
    }

    void Player::changeStateTo(ServerPlayerState state)
    {
        if (_state == state) return;

        qDebug() << "Player: state changed from" << int(_state) << "to" << int(state);
        _state = state;
        Q_EMIT stateChanged(state);

        if (state == ServerPlayerState::Playing)
        {
            emitStartedPlaying(_nowPlaying);
        }
    }

    QSharedPointer<QueueEntry const> Player::nowPlaying() const
    {
        return _nowPlaying;
    }

    uint Player::nowPlayingQID() const
    {
        auto entry = _nowPlaying;
        return entry ? entry->queueID() : 0;
    }

    qint64 Player::playPosition() const
    {
        return _playPosition;
    }

    quint32 Player::userPlayingFor() const
    {
        return _userPlayingFor;
    }

    PlayerQueue& Player::queue()
    {
        return _queue;
    }

    Resolver& Player::resolver()
    {
        return *_resolver;
    }

    void Player::playPause()
    {
        if (_state == ServerPlayerState::Playing)
            pause();
        else
            play();
    }

    void Player::play()
    {
        switch (_state)
        {
            case ServerPlayerState::Stopped:
                startNext(false, true);
                break;
            case ServerPlayerState::Paused:
                if (_currentInstance)
                {
                    bool atTheBeginning = _currentInstance->position() == 0;
                    
                    _currentInstance->play(); /* resume paused track */
                    changeStateTo(ServerPlayerState::Playing);

                    if (atTheBeginning)
                        emitStartedPlaying(_nowPlaying);
                }
                break;
            case ServerPlayerState::Playing:
                break; /* already playing */
        }
    }

    void Player::pause()
    {
        switch (_state)
        {
            case ServerPlayerState::Stopped:
            case ServerPlayerState::Paused:
                break; /* no effect */
            case ServerPlayerState::Playing:
                if (_currentInstance)
                {
                    _currentInstance->pause();
                    changeStateTo(ServerPlayerState::Paused);
                }
                break;
        }
    }

    void Player::skip()
    {
        bool mustPlay;

        switch (_state)
        {
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

    void Player::seekTo(qint64 position)
    {
        if (_state != ServerPlayerState::Playing && _state != ServerPlayerState::Paused)
            return;

        if (_currentInstance)
            _currentInstance->seekTo(position);
    }

    void Player::setVolume(int volume)
    {
        if (volume < 0 || volume > 100) return;
        if (volume == _volume) return;

        _volume = volume;

        if (_currentInstance)
            _currentInstance->setVolume(volume);

        Q_EMIT volumeChanged(volume);
    }

    void Player::setUserPlayingFor(quint32 user)
    {
        if (_userPlayingFor == user) return; /* no change */

        _userPlayingFor = user;
        Q_EMIT userPlayingForChanged(user);
    }

    void Player::startNext(bool stopCurrent, bool playNext)
    {
        qDebug() << "Player::startNext(" << stopCurrent << "," << playNext << ") called";

        PlayerInstance* oldCurrentInstance = _currentInstance;
        QSharedPointer<QueueEntry> oldNowPlaying = _nowPlaying;

        if (_currentInstance)
            moveCurrentInstanceToOldInstanceSlot();

        /* Take control of the next instance so it won't be used for preparing the next
           track as soon as we dequeue the track we want to play right now */
        PlayerInstance* nextInstance = _nextInstance;
        _nextInstance = nullptr;

        /* find next track to play and start it */
        QSharedPointer<QueueEntry> nextTrack = nullptr;
        while (!_queue.empty())
        {
            if (_queue.firstEntryIsBarrier())
                break; /* this will cause playback to stop because next == nullptr */

            auto entry = _queue.dequeue();
            if (!entry->isTrack())
            {
                if (entry->kind() == QueueEntryKind::Break)
                {
                    break; /* this will cause playback to stop because next == nullptr */
                }
                else /* unknown non-track thing, let's just ignore it */
                {
                    continue;
                }
            }

            putInHistoryOrder(entry);
            bool ok = tryStartNextTrack(nextInstance, entry, playNext);
            if (ok)
            {
                nextTrack = entry;
                break;
            }
            else
            {
                qWarning() << "Player: failed to load/start track with queue ID"
                           << entry->queueID();

                addToHistory(entry, 0, true, false); /* register track as not played */
            }
        }

        if (stopCurrent && oldCurrentInstance)
        {
            oldCurrentInstance->stop();
        }

        ServerPlayerState newState;
        if (nextTrack)
        {
            newState = playNext ? ServerPlayerState::Playing : ServerPlayerState::Paused;

            /* try to figure out track length, and if possible tag, artist... */
            nextTrack->checkTrackData(*_resolver);
        }
        else
        {
            /* we stop because we don't have a track to play */
            newState = ServerPlayerState::Stopped;

            /* this instance ended up not being used for playing a track, don't lose it */
            moveToOldInstanceSlot(nextInstance);
        }

        _nowPlaying = nextTrack;

        if (!nextTrack)
            _playPosition = 0;

        if (oldNowPlaying || nextTrack)
            Q_EMIT currentTrackChanged(nextTrack);

        changeStateTo(newState);

        if (nextTrack && playNext)
            emitStartedPlaying(_nowPlaying);
    }

    void Player::instancePlaying(PlayerInstance* instance)
    {
        if (instance != _currentInstance) return;

        changeStateTo(ServerPlayerState::Playing);
    }

    void Player::instancePaused(PlayerInstance* instance)
    {
        if (instance != _currentInstance) return;

        changeStateTo(ServerPlayerState::Paused);
    }

    void Player::instancePositionChanged(PlayerInstance* instance, qint64 position)
    {
        if (instance != _currentInstance) return;

        if (_state == ServerPlayerState::Stopped)
        {
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
            prepareForFirstTrackFromQueue();
    }

    void Player::instancePlaybackError(PlayerInstance* instance)
    {
        /* register track as not played */
        addToHistory(instance->track(), 0, true, false);
    }

    void Player::instanceTrackFinished(PlayerInstance* instance)
    {
        auto track = instance->track();
        bool hadSeek = instance->hadSeek();
        addToHistory(track, 1000, false, hadSeek);

        if (instance != _currentInstance) return;

        switch (_state)
        {
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

    void Player::instanceStoppedEarly(PlayerInstance* instance, qint64 position)
    {
        auto track = instance->track();
        bool hadSeek = instance->hadSeek();
        auto permillage = calcPermillagePlayed(track, position, hadSeek);

        addToHistory(instance->track(), permillage, false, hadSeek);
    }

    void Player::firstTrackInQueueChanged(int index, uint queueId)
    {
        if (index < 0) return;

        if (_preloader.havePreloadedFileQuickCheck(queueId))
            prepareForFirstTrackFromQueue();
    }

    void Player::trackPreloaded(uint queueId)
    {
        if (queueId == _queue.firstTrackQueueId())
            prepareForFirstTrackFromQueue();
    }

    void Player::makeSureOneOldInstanceSlotIsFree()
    {
        if (!_oldInstance1 || !_oldInstance2)
            return;

        if (!_nextInstance && _oldInstance1->availableForNewTrack())
        {
            qDebug() << "moving old player instance 1 to next instance";
            _nextInstance = _oldInstance1;
            _oldInstance1 = nullptr;
        }
        else if (!_nextInstance && _oldInstance2->availableForNewTrack())
        {
            qDebug() << "moving old player instance 2 to next instance";
            _nextInstance = _oldInstance2;
            _oldInstance2 = nullptr;
        }
        else
        {
            qDebug() << "old player instance 2 will be deleted";
            _oldInstance2->deleteAfterStopped();
            _oldInstance2 = nullptr;
        }
    }

    void Player::moveCurrentInstanceToOldInstanceSlot()
    {
        moveToOldInstanceSlot(_currentInstance);
        _currentInstance = nullptr;
    }

    void Player::moveToOldInstanceSlot(PlayerInstance* instance)
    {
        if (!instance)
            return;

        makeSureOneOldInstanceSlotIsFree();

        if (!_oldInstance1)
        {
            _oldInstance1 = instance;
        }
        else if (!_oldInstance2)
        {
            _oldInstance2 = instance;
        }
        else
        {
            qWarning() << "will have to delete instance because no slot is free";
            instance->deleteAfterStopped();
        }
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
            [=](bool endOfTrackComingUp)
            {
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

    void Player::prepareForFirstTrackFromQueue()
    {
        auto nextTrack = _queue.peekFirstTrackEntry();
        if (!nextTrack || !nextTrack->isTrack())
            return; /* no next track to prepare for now */

        if (!_nextInstance)
        {
            if (_oldInstance1 && _oldInstance1->availableForNewTrack())
            {
                _nextInstance = _oldInstance1;
                _oldInstance1 = nullptr;
            }
            else if (_oldInstance2 && _oldInstance2->availableForNewTrack())
            {
                _nextInstance = _oldInstance2;
                _oldInstance2 = nullptr;
            }
            else
            {
                _nextInstance = createNewPlayerInstance();
            }
        }

        tryPrepareTrack(_nextInstance, nextTrack, true); /* ignore the result */
    }

    bool Player::tryPrepareTrack(PlayerInstance* playerInstance,
                                 QSharedPointer<QueueEntry> entry,
                                 bool onlyIfPreloaded)
    {
        if (!entry || !entry->isTrack())
            return false; /* argument should be a track */

        if (!playerInstance->availableForNewTrack())
        {
            qWarning() << "instance not available for new track!";
            return false;
        }

        if (playerInstance->track() == entry && playerInstance->trackSetSuccessfully())
            return true; /* already prepared */

        playerInstance->setTrack(entry, onlyIfPreloaded);
        return playerInstance->trackSetSuccessfully();
    }

    bool Player::tryStartNextTrack(PlayerInstance* playerInstance,
                                   QSharedPointer<QueueEntry> entry,
                                   bool startPlaying)
    {
        if (!entry || !entry->isTrack())
            return false; /* argument should be a track */

        /* make sure we are prepared */
        bool prepared = tryPrepareTrack(playerInstance, entry, false);
        if (!prepared)
            return false; /* preparation went wrong */

        _currentInstance = playerInstance;
        _playPosition = 0;
        playerInstance->setVolume(_volume);

        if (startPlaying)
            playerInstance->play();

        return true;
    }

    void Player::emitStartedPlaying(QSharedPointer<QueueEntry> queueEntry)
    {
        if (!queueEntry) return;

        auto startTime = queueEntry->started();
        int lengthInSeconds =
                static_cast<int>(queueEntry->lengthInMilliseconds() / 1000);

        Q_EMIT startedPlaying(_userPlayingFor, startTime, queueEntry->title(),
                              queueEntry->artist(), queueEntry->album(),
                              queueEntry->albumArtist(), lengthInSeconds);
    }

    void Player::putInHistoryOrder(QSharedPointer<QueueEntry> entry)
    {
        _historyOrder.enqueue(entry->queueID());
    }

    void Player::addToHistory(QSharedPointer<QueueEntry> entry, int permillage,
                              bool hadError, bool hadSeek)
    {
        entry->setEndedNow(); /* register end time */

        if (!entry->isTrack())
            return; /* don't put breakpoints in the history */

        if (_historyOrder.isEmpty())
        {
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
        if (!ended.isValid())
        {
            if (permillage > 0)
                ended = QDateTime::currentDateTimeUtc();
            else
                ended = started;
        }

        Nullable<FileHash> hash = entry->hash();

        auto historyEntry =
            QSharedPointer<RecentHistoryEntry>::create(
                queueID, hash.value(), userPlayedFor, started, ended,
                hadError, hadSeek, permillage
            );

        if (_historyOrder.isEmpty() || _historyOrder.first() != queueID)
        {
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

    void Player::performHistoryActions(QSharedPointer<RecentHistoryEntry> historyEntry)
    {
        _queue.addToHistory(historyEntry);

        Q_EMIT newHistoryEntry(historyEntry);
    }

    /* permillage (for lack of a better name) is like percentage, but with factor 1000
       instead of 100 */
    int Player::calcPermillagePlayed(QSharedPointer<QueueEntry> track,
                                     qint64 positionReached,
                                     bool seeked)
    {
        if (seeked) return -1;
        if (!track) return -2;

        auto msLength = track->lengthInMilliseconds();
        if (msLength < 0) return -3;

        return qBound(0, int(positionReached * 1000 / msLength), 1000);
    }
}
