/*
    Copyright (C) 2018-2023, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_SCROBBLER_H
#define PMP_SCROBBLER_H

#include "common/future.h"
#include "common/scrobblerstatus.h"

#include "result.h"
#include "scrobblingbackend.h"
#include "trackinfoprovider.h"
#include "tracktoscrobble.h"

#include <QObject>
#include <QQueue>
#include <QSharedPointer>

QT_FORWARD_DECLARE_CLASS(QTimer)

namespace PMP::Server
{
    class ScrobblingDataProvider;

    class Scrobbler : public QObject
    {
        Q_OBJECT
    public:
        Scrobbler(QObject* parent, QSharedPointer<ScrobblingDataProvider> dataProvider,
                  ScrobblingBackend* backend, TrackInfoProvider* trackInfoProvider);

        ScrobblerStatus status() const { return _status; }

        SimpleFuture<Result> authenticateWithCredentials(QString usernameOrEmail,
                                                         QString password);

    public Q_SLOTS:
        void wakeUp();
        void nowPlayingNothing();
        void nowPlayingTrack(QDateTime startTime, PMP::Server::ScrobblingTrack track);

    Q_SIGNALS:
        void statusChanged(PMP::ScrobblerStatus status);

    private Q_SLOTS:
        void timeoutTimerTimedOut();
        void backoffTimerTimedOut();
        void gotNowPlayingResult(bool success);
        void gotScrobbleResult(PMP::Server::ScrobbleResult result);
        void backendStateChanged(PMP::Server::ScrobblingBackendState newState,
                                 PMP::Server::ScrobblingBackendState oldState);
        void serviceTemporarilyUnavailable();
        void reevaluateStatus();

    private:
        void checkIfWeHaveSomethingToDo();
        void initializeBackend();
        void sendScrobblesOrNowPlaying();
        void sendNowPlaying();
        void sendNextScrobble();
        void startBackoffTimer(int initialBackoffMilliseconds);
        void reinsertPendingScrobbleAtFrontOfQueue();

        QSharedPointer<ScrobblingDataProvider> _dataProvider;
        ScrobblingBackend* _backend;
        TrackInfoProvider* _trackInfoProvider;
        ScrobblerStatus _status;
        QQueue<QSharedPointer<TrackToScrobble>> _tracksToScrobble;
        QSharedPointer<TrackToScrobble> _pendingScrobble;
        QTimer* _timeoutTimer;
        QTimer* _backoffTimer;
        ScrobblingTrack _nowPlayingTrack;
        QDateTime _nowPlayingStartTime;
        int _backoffMilliseconds;
        bool _nowPlayingPresent;
        bool _nowPlayingSent;
        bool _nowPlayingDone;
    };
}
#endif
