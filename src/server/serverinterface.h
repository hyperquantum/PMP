/*
    Copyright (C) 2020-2021, Kevin Andre <hyperquantum@gmail.com>

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
#include "common/resultmessageerrorcode.h"
#include "common/startstopeventstatus.h"

#include "result.h"

#include <QObject>
#include <QHash>
#include <QString>
#include <QUuid>

#include <functional>

namespace PMP
{
    class FileHash;
    class Generator;
    class Player;
    class PlayerQueue;
    class QueueEntry;
    class Server;
    class ServerSettings;

    enum class QueueIndexType { Front, End };

    class ServerInterface : public QObject
    {
        Q_OBJECT
    public:
        ServerInterface(ServerSettings* serverSettings, Server* server, Player* player,
                        Generator* generator);

        QUuid getServerUuid() const;
        QString getServerCaption() const;

        bool isLoggedIn() const { return _userLoggedIn > 0; }
        void setLoggedIn(quint32 userId, QString userLogin);

        void reloadServerSettings(uint clientReference);

        void switchToPersonalMode();
        void switchToPublicMode();

        void play();
        void pause();
        void skip();
        void seekTo(qint64 positionMilliseconds);

        void setVolume(int volumePercentage);

        Result enqueue(FileHash hash);
        Result insertAtFront(FileHash hash);
        Result insertBreakAtFrontIfNotExists();
        Result insertBreak(QueueIndexType indexType, int offset, quint32 clientReference);
        Result duplicateQueueEntry(uint id, quint32 clientReference);
        Result insertAtIndex(qint32 index,
                             std::function<QueueEntry* (uint)> queueEntryCreator,
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

        void serverSettingsReloadResultEvent(uint clientReference,
                                             ResultMessageErrorCode errorCode);

        void queueEntryAddedWithoutReference(quint32 offset, quint32 queueId);
        void queueEntryAddedWithReference(quint32 offset, quint32 queueId,
                                          quint32 clientReference);

        void dynamicModeStatusEvent(StartStopEventStatus dynamicModeStatus,
                                    int noRepetitionSpanSeconds);

        void dynamicModeWaveStatusEvent(StartStopEventStatus waveStatus,
                                        quint32 user,
                                        int waveDeliveredCount,
                                        int waveTotalCount);

    private Q_SLOTS:
        void onQueueEntryAdded(quint32 offset, quint32 queueId);

        void onDynamicModeStatusChanged();
        void onDynamicModeNoRepetitionSpanChanged();
        void onDynamicModeWaveStarted();
        void onDynamicModeWaveProgress(int tracksDelivered, int tracksTotal);
        void onDynamicModeWaveEnded();

    private:
        int calculateQueueIndex(PlayerQueue const& queue, QueueIndexType indexType,
                                int offset);
        std::function<void (uint)> createQueueInsertionIdNotifier(
                                                                 quint32 clientReference);

        quint32 _userLoggedIn;
        QString _userLoggedInName;
        ServerSettings* _serverSettings;
        Server* _server;
        Player* _player;
        Generator* _generator;
        QHash<quint32, quint32> _queueEntryInsertionsPending;
    };
}
#endif
