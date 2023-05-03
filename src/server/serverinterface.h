/*
    Copyright (C) 2020-2023, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_SERVERINTERFACE_H
#define PMP_SERVERINTERFACE_H

#include "common/filehash.h"
#include "common/future.h"
#include "common/queueindextype.h"
#include "common/resultmessageerrorcode.h"
#include "common/specialqueueitemtype.h"
#include "common/startstopeventstatus.h"
#include "common/versioninfo.h"

#include "hashstats.h"
#include "result.h"
#include "serverplayerstate.h"

#include <QObject>
#include <QHash>
#include <QSet>
#include <QString>
#include <QUuid>

#include <functional>

namespace PMP::Server
{
    class DelayedStart;
    class Generator;
    class HashIdRegistrar;
    class History;
    class Player;
    class PlayerQueue;
    class QueueEntry;
    class ServerSettings;
    class TcpServer;
    class Users;

    struct PlayerStateOverview
    {
        quint64 trackPosition;
        quint32 nowPlayingQueueId;
        qint32 queueLength;
        ServerPlayerState playerState;
        quint8 volume;
        bool delayedStartActive;
    };

    class ServerInterface : public QObject
    {
        Q_OBJECT
    public:
        ServerInterface(ServerSettings* serverSettings, TcpServer* server, Player* player,
                        Generator* generator, History* history,
                        HashIdRegistrar* hashIdRegistrar, Users* users,
                        DelayedStart* delayedStart);

        ~ServerInterface();

        QUuid getServerUuid() const;
        QString getServerCaption() const;
        VersionInfo getServerVersionInfo() const;

        ResultOrError<QUuid, Result> getDatabaseUuid() const;

        quint32 userLoggedIn() const { return _userLoggedIn; }
        bool isLoggedIn() const { return _userLoggedIn > 0; }
        void setLoggedIn(quint32 userId, QString userLogin);

        SimpleFuture<ResultMessageErrorCode> reloadServerSettings();

        void switchToPersonalMode();
        void switchToPublicMode();

        Result activateDelayedStart(qint64 delayMilliseconds);
        Result deactivateDelayedStart();
        bool delayedStartActive() const;
        qint64 getDelayedStartTimeRemainingMilliseconds() const;

        void play();
        void pause();
        void skip();
        void seekTo(qint64 positionMilliseconds);

        void setVolume(int volumePercentage);

        PlayerStateOverview getPlayerStateOverview();

        Future<QVector<QString>, Result> getPossibleFilenamesForQueueEntry(uint id);

        Result insertTrackAtEnd(FileHash hash);
        Result insertTrackAtFront(FileHash hash);
        Result insertBreakAtFrontIfNotExists();
        Result insertTrack(FileHash hash, int index, quint32 clientReference);
        Result insertSpecialQueueItem(SpecialQueueItemType itemType,
                                      QueueIndexType indexType, int index,
                                      quint32 clientReference);
        Result duplicateQueueEntry(uint id, quint32 clientReference);
        void moveQueueEntry(uint id, int upDownOffset);
        void removeQueueEntry(uint id);
        void trimQueue();
        void requestQueueExpansion();

        void requestDynamicModeStatus();
        void enableDynamicMode();
        void disableDynamicMode();
        void startDynamicModeWave();
        void terminateDynamicModeWave();
        void setTrackRepetitionAvoidanceSeconds(int seconds);

        void startFullIndexation();
        bool isFullIndexationRunning();

        void requestHashUserData(quint32 userId, QVector<FileHash> hashes);

        void shutDownServer();
        void shutDownServer(QString serverPassword);

    Q_SIGNALS:
        void serverCaptionChanged();
        void serverClockTimeSendingPulse();
        void serverShuttingDown();

        void delayedStartActiveChanged();

        void queueEntryAddedWithoutReference(qint32 offset, quint32 queueId);
        void queueEntryAddedWithReference(qint32 offset, quint32 queueId,
                                          quint32 clientReference);

        void dynamicModeStatusEvent(StartStopEventStatus dynamicModeStatus,
                                    int noRepetitionSpanSeconds);

        void dynamicModeWaveStatusEvent(StartStopEventStatus waveStatus,
                                        quint32 user,
                                        int waveDeliveredCount,
                                        int waveTotalCount);

        void fullIndexationRunStatusChanged(bool running);

        void hashUserDataChangedOrAvailable(quint32 userId, QVector<HashStats> tracks);

    private Q_SLOTS:
        void onQueueEntryAdded(qint32 offset, quint32 queueId);

        void onDynamicModeStatusChanged();
        void onDynamicModeNoRepetitionSpanChanged();
        void onDynamicModeWaveStarted();
        void onDynamicModeWaveProgress(int tracksDelivered, int tracksTotal);
        void onDynamicModeWaveEnded();

        void onHashStatisticsChanged(quint32 userId, QVector<uint> hashIds);

    private:
        int toNormalIndex(PlayerQueue const& queue, QueueIndexType indexType,
                                int index);
        std::function<void (uint)> createQueueInsertionIdNotifier(
                                                                 quint32 clientReference);
        Result insertAtIndex(qint32 index,
                       std::function<QSharedPointer<QueueEntry> (uint)> queueEntryCreator,
                       quint32 clientReference);
        void addUserHashDataNotification(quint32 userId, QVector<uint> hashIds);
        void sendUserHashDataNotifications(quint32 userId);

        quint32 _userLoggedIn;
        QString _userLoggedInName;
        ServerSettings* _serverSettings;
        TcpServer* _server;
        Player* _player;
        Generator* _generator;
        History* _history;
        HashIdRegistrar* _hashIdRegistrar;
        Users* _users;
        DelayedStart* _delayedStart;
        QHash<quint32, quint32> _queueEntryInsertionsPending;
        QHash<quint32, QSet<uint>> _userHashDataNotificationsPending;
        QHash<quint32, bool> _userHashDataNotificationTimerRunning;
    };
}
#endif
