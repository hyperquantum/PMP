/*
    Copyright (C) 2020-2022, Kevin Andre <hyperquantum@gmail.com>

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

#include "result.h"
#include "serverplayerstate.h"

#include <QObject>
#include <QHash>
#include <QString>
#include <QUuid>

#include <functional>

namespace PMP
{
    class DelayedStart;
    class FileHash;
    class Generator;
    class Player;
    class PlayerQueue;
    class QueueEntry;
    class Server;
    class ServerSettings;

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
        ServerInterface(ServerSettings* serverSettings, Server* server,
                        uint connectionReference, Player* player, Generator* generator,
                        DelayedStart* delayedStart);

        ~ServerInterface();

        QUuid getServerUuid() const;
        QString getServerCaption() const;

        ResultOrError<QUuid, Result> getDatabaseUuid() const;

        bool isLoggedIn() const { return _userLoggedIn > 0; }
        quint32 userLoggedIn() const { return _userLoggedIn; }
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

        Future<QList<QString>, Result> getPossibleFilenamesForQueueEntry(uint id);

        Result enqueue(FileHash hash);
        Result insertAtFront(FileHash hash);
        Result insertBreakAtFrontIfNotExists();
        Result insertSpecialQueueItem(SpecialQueueItemType itemType,
                                      QueueIndexType indexType, int index,
                                      quint32 clientReference);
        Result duplicateQueueEntry(uint id, quint32 clientReference);
        Result insertAtIndex(qint32 index,
                       std::function<QSharedPointer<QueueEntry> (uint)> queueEntryCreator,
                       quint32 clientReference);
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

    private Q_SLOTS:
        void onQueueEntryAdded(qint32 offset, quint32 queueId);

        void onDynamicModeStatusChanged();
        void onDynamicModeNoRepetitionSpanChanged();
        void onDynamicModeWaveStarted();
        void onDynamicModeWaveProgress(int tracksDelivered, int tracksTotal);
        void onDynamicModeWaveEnded();

    private:
        int toNormalIndex(PlayerQueue const& queue, QueueIndexType indexType, int index);
        std::function<void (uint)> createQueueInsertionIdNotifier(
                                                                 quint32 clientReference);

        const uint _connectionReference;    
        quint32 _userLoggedIn;
        QString _userLoggedInName;
        ServerSettings* _serverSettings;
        Server* _server;
        Player* _player;
        Generator* _generator;
        DelayedStart* _delayedStart;
        QHash<quint32, quint32> _queueEntryInsertionsPending;
    };
}
#endif
