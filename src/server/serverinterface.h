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

#include <QObject>
#include <QString>
#include <QUuid>

namespace PMP
{
    class FileHash;
    class Generator;
    class Player;
    class QueueEntry;
    class Server;
    class ServerSettings;

    class ServerInterface : public QObject
    {
        Q_OBJECT
    public:
        ServerInterface(ServerSettings* serverSettings, Server* server,
                        uint connectionReference, Player* player, Generator* generator);

        QUuid getServerUuid() const;

        quint32 userLoggedIn() const { return _userLoggedIn; }
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

        void enqueue(FileHash hash);
        void insertAtFront(FileHash hash);
        void insertBreakAtFront();
        void insertAtIndex(quint32 index, QueueEntry* entry);
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
        void serverClockTimeSendingPulse();
        void serverShuttingDown();

        void serverSettingsReloadResultEvent(uint clientReference,
                                             ResultMessageErrorCode errorCode);

        void dynamicModeStatusEvent(StartStopEventStatus dynamicModeStatus,
                                    int noRepetitionSpanSeconds);

        void dynamicModeWaveStatusEvent(StartStopEventStatus waveStatus,
                                        quint32 user,
                                        int waveDeliveredCount,
                                        int waveTotalCount);

    private Q_SLOTS:
        void onDynamicModeStatusChanged();
        void onDynamicModeNoRepetitionSpanChanged();
        void onDynamicModeWaveStarted();
        void onDynamicModeWaveProgress(int tracksDelivered, int tracksTotal);
        void onDynamicModeWaveEnded();

    private:
        const uint _connectionReference;
        quint32 _userLoggedIn;
        QString _userLoggedInName;
        ServerSettings* _serverSettings;
        Server* _server;
        Player* _player;
        Generator* _generator;
    };
}
#endif
