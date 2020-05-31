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

#ifndef PMP_PLAYER_H
#define PMP_PLAYER_H

#include "playerqueue.h"
#include "preloader.h"
#include "serverplayerstate.h"

#include <QDateTime>
#include <QHash>
#include <QMediaPlayer>
#include <QQueue>

namespace PMP {

    class Resolver;

    class PlayerInstance : public QObject {
        Q_OBJECT
    public:
        PlayerInstance(QObject* parent, int identifier, Preloader* preloader,
                       Resolver* resolver);

        QueueEntry* track() const { return _track; }
        bool availableForNewTrack() const;

        bool trackSetSuccessfully() const { return _mediaSet; }
        bool endOfTrackComingUp() const { return _endOfTrackComingUp; }
        bool hadSeek() const { return _hadSeek; }

    public Q_SLOTS:
        void setVolume(int volume);
        void setTrack(QueueEntry* queueEntry, bool onlyIfPreloaded);
        void play();
        void pause();
        void stop();
        void seekTo(qint64 position);

        void deleteAfterStopped();

    Q_SIGNALS:
        void playing();
        void paused();
        void positionChanged(qint64 position);
        void endOfTrackComingUpChanged(bool endOfTrackComingUp);
        void playbackError();
        void trackFinished();
        void stoppedEarly(qint64 position);

    private Q_SLOTS:
        void internalStateChanged(QMediaPlayer::State state);
        void internalMediaStatusChanged(QMediaPlayer::MediaStatus);
        void internalPositionChanged(qint64 position);
        void internalDurationChanged(qint64 duration);

    private:
        void updateEndOfTrackComingUpFlag();

        QMediaPlayer* _player;
        Preloader* _preloader;
        Resolver* _resolver;
        QueueEntry* _track;
        PreloadedFile _preloadedFile;
        qint64 _positionWhenStopped;
        int _identifier;
        bool _mediaSet;
        bool _endOfTrackComingUp;
        bool _hadSeek;
        bool _deleteAfterStopped;
    };

    class Player : public QObject {
        Q_OBJECT
    public:
        Player(QObject* parent, Resolver* resolver, int defaultVolume);

        int volume() const;

        bool playing() const;
        ServerPlayerState state() const;
        QueueEntry const* nowPlaying() const;
        uint nowPlayingQID() const;
        qint64 playPosition() const;
        quint32 userPlayingFor() const;

        PlayerQueue& queue();

        Resolver& resolver();

    public Q_SLOTS:

        void playPause();
        void play();
        void pause();

        /** Skip the currently playing/paused track. Does nothing when not currently
            playing. */
        void skip();

        void seekTo(qint64 position);

        void setVolume(int volume);

        void setUserPlayingFor(quint32 user);

    Q_SIGNALS:
        void stateChanged(ServerPlayerState state);
        void currentTrackChanged(QueueEntry const* newTrack);
        void positionChanged(qint64 position);
        void volumeChanged(int volume);
        void userPlayingForChanged(quint32 user);
        void newHistoryEntry(QSharedPointer<PlayerHistoryEntry> entry);

        /*! Emitted when the queue is empty and the current track is finished. */
        void finished();

    private Q_SLOTS:
        void changeStateTo(ServerPlayerState state);

        void instancePlaying(PlayerInstance* instance);
        void instancePaused(PlayerInstance* instance);
        void instancePositionChanged(PlayerInstance* instance, qint64 position);
        void instanceEndOfTrackComingUpChanged(PlayerInstance* instance,
                                               bool endOfTrackComingUp);
        void instancePlaybackError(PlayerInstance* instance);
        void instanceTrackFinished(PlayerInstance* instance);
        void instanceStoppedEarly(PlayerInstance* instance, qint64 position);

        void firstTrackInQueueChanged(int index, uint queueId);
        void trackPreloaded(uint queueId);

    private:
        void makeSureOneOldInstanceSlotIsFree();
        void moveCurrentInstanceToOldInstanceSlot();
        void moveToOldInstanceSlot(PlayerInstance* instance);
        bool startNext(bool stopCurrent, bool playNext);
        PlayerInstance* createNewPlayerInstance();
        void prepareForFirstTrackFromQueue();
        bool tryPrepareTrack(PlayerInstance* playerInstance, QueueEntry* entry,
                             bool onlyIfPreloaded);
        bool tryStartNextTrack(PlayerInstance* playerInstance, QueueEntry* entry,
                               bool startPlaying);
        void putInHistoryOrder(QueueEntry* entry);
        void addToHistory(QueueEntry* entry, int permillage, bool hadError, bool hadSeek);
        void performHistoryActions(QSharedPointer<PlayerHistoryEntry> historyEntry);
        static int calcPermillagePlayed(QueueEntry* track, qint64 positionReached,
                                        bool seeked);

        PlayerInstance* _oldInstance1;
        PlayerInstance* _oldInstance2;
        PlayerInstance* _currentInstance;
        PlayerInstance* _nextInstance;
        Resolver* _resolver;
        PlayerQueue _queue;
        Preloader _preloader;
        QueueEntry* _nowPlaying;
        QQueue<uint> _historyOrder;
        QHash<uint, QSharedPointer<PlayerHistoryEntry>> _pendingHistory;
        int _instanceIdentifier;
        int _volume;
        qint64 _playPosition;
        ServerPlayerState _state;
        quint32 _userPlayingFor;
    };
}
#endif
