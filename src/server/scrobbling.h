/*
    Copyright (C) 2019-2023, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_SCROBBLING_H
#define PMP_SCROBBLING_H

#include "common/future.h"
#include "common/scrobblerstatus.h"
#include "common/scrobblingprovider.h"

#include "result.h"
#include "scrobblingtrack.h"

#include <QDateTime>
#include <QHash>
#include <QObject>
#include <QThread>

namespace PMP::Server
{
    class ScrobblingHost;
    class TrackInfoProvider;

    class GlobalScrobblingController : public QObject
    {
        Q_OBJECT
    public:
        GlobalScrobblingController();

    public Q_SLOTS:
        void enableScrobbling();
        void wakeUp(uint userId);
        void updateNowPlaying(uint userId, QDateTime startTime,
                              PMP::Server::ScrobblingTrack track);

    Q_SIGNALS:
        void enableScrobblingRequested();
        void wakeUpRequested(uint userId);

        void nowPlayingUpdateRequested(uint userId, QDateTime startTime,
                                       PMP::Server::ScrobblingTrack track);
    };

    class UserScrobblingController : public QObject
    {
        Q_OBJECT
    public:
        UserScrobblingController(uint userId);

    public Q_SLOTS:
        void wakeUp();
        void setScrobblingProviderEnabled(PMP::ScrobblingProvider provider, bool enabled);
        void requestScrobblingProviderInfo();

    Q_SIGNALS:
        void wakeUpRequested(uint userId);
        void providerEnableOrDisableRequested(uint userId,
                                              PMP::ScrobblingProvider provider,
                                              bool enabled);
        void scrobblingProviderInfoRequested(uint userId);

        void scrobblingProviderInfo(PMP::ScrobblingProvider provider,
                                    PMP::ScrobblerStatus status, bool enabled);
        void scrobblerStatusChanged(PMP::ScrobblingProvider provider,
                                    PMP::ScrobblerStatus status);
        void scrobblingProviderEnabledChanged(PMP::ScrobblingProvider provider,
                                              bool enabled);

    private:
        uint _userId;
    };

    class Scrobbling : public QObject
    {
        Q_OBJECT
    public:
        explicit Scrobbling(QObject* parent, TrackInfoProvider* trackInfoProvider);
        ~Scrobbling();

        GlobalScrobblingController* getController();
        UserScrobblingController* getControllerForUser(uint userId);

        SimpleFuture<Result> authenticateForProvider(uint userId,
                                                     ScrobblingProvider provider,
                                                     QString user,
                                                     QString password);

    private:
        TrackInfoProvider* _trackInfoProvider;
        ScrobblingHost* _host;
        QThread _thread;
        GlobalScrobblingController* _controller;
        QHash<uint, UserScrobblingController*> _userControllers;
    };
}
#endif
