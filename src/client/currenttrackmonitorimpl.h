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

#ifndef PMP_CURRENTTRACKMONITORIMPL_H
#define PMP_CURRENTTRACKMONITORIMPL_H

#include "currenttrackmonitor.h"

#include <QElapsedTimer>

namespace PMP::Client
{
    class QueueEntryInfoStorage;
    class ServerConnection;

    class CurrentTrackMonitorImpl : public CurrentTrackMonitor
    {
        Q_OBJECT
    public:
        explicit CurrentTrackMonitorImpl(QueueEntryInfoStorage* queueEntryInfoStorage,
                                         ServerConnection* connection);

        PlayerState playerState() const override;

        TriBool isTrackPresent() const override;
        quint32 currentQueueId() const override;
        qint64 currentTrackProgressMilliseconds() const override;

        FileHash currentTrackHash() const override;

        QString currentTrackTitle() const override;
        QString currentTrackArtist() const override;
        QString currentTrackPossibleFilename() const override;
        qint64 currentTrackLengthMilliseconds() const override;

    public Q_SLOTS:
        void seekTo(qint64 positionInMilliseconds) override;

    private Q_SLOTS:
        void connected();
        void connectionBroken();
        void receivedPlayerState(PlayerState state, quint8 volume, quint32 queueLength,
                                 quint32 nowPlayingQueueId, quint64 nowPlayingPosition);
        void tracksChanged(QList<quint32> queueIds);

    private:
        void changeCurrentQueueId(quint32 queueId);
        void updateTrackFields(bool isNewTrack);
        void changeCurrentTrackPosition(qint64 positionMilliseconds);
        void emitCalculatedTrackProgress();
        void clearTrackInfo();

        QueueEntryInfoStorage* _queueEntryInfoStorage;
        ServerConnection* _connection;
        PlayerState _playerState;
        quint32 _currentQueueId;
        QElapsedTimer _progressTimer;
        qint64 _progressAtTimerStart;
        FileHash _currentHash;
        bool _haveReceivedCurrentTrack;
        QString _currentTrackTitle;
        QString _currentTrackArtist;
        QString _currentTrackPossibleFilename;
        qint64 _currentTrackLengthMilliseconds;
    };
}
#endif
