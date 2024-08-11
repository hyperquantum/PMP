/*
    Copyright (C) 2019-2024, Kevin Andre <hyperquantum@gmail.com>

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

#include "scrobbling.h"

#include "common/async.h"

#include "scrobblinghost.h"

#include <QtDebug>

namespace PMP::Server
{
    GlobalScrobblingController::GlobalScrobblingController()
    {
        //
    }

    void GlobalScrobblingController::enableScrobbling()
    {
        Q_EMIT enableScrobblingRequested();
    }

    void GlobalScrobblingController::wakeUp(uint userId)
    {
        Q_EMIT wakeUpRequested(userId);
    }

    void GlobalScrobblingController::updateNowPlaying(uint userId, QDateTime startTime,
                                                      ScrobblingTrack track)
    {
        Q_EMIT nowPlayingUpdateRequested(userId, startTime, track);
    }

    /* ============================================================================ */

    UserScrobblingController::UserScrobblingController(uint userId)
     : _userId(userId)
    {
        //
    }

    void UserScrobblingController::wakeUp()
    {
        qDebug() << "UserScrobblingController::wakeUp called for user" << _userId;
        Q_EMIT wakeUpRequested(_userId);
    }

    void UserScrobblingController::setScrobblingProviderEnabled(
                                                              ScrobblingProvider provider,
                                                              bool enabled)
    {
        qDebug()
            << "UserScrobblingController::setScrobblingProviderEnabled called for user"
            << _userId << "and provider" << provider << "with enabled=" << enabled;

        Q_EMIT providerEnableOrDisableRequested(_userId, provider, enabled);
    }

    void UserScrobblingController::requestScrobblingProviderInfo()
    {
        qDebug()
            << "UserScrobblingController::requestScrobblingProviderInfo called for user"
            << _userId;

        Q_EMIT scrobblingProviderInfoRequested(_userId);
    }

    /* ============================================================================ */

    Scrobbling::Scrobbling(QObject* parent, TrackInfoProvider* trackInfoProvider)
      : QObject(parent),
        _trackInfoProvider(trackInfoProvider),
        _host(nullptr),
        _controller(nullptr)
    {
        _thread.setObjectName("ScrobblingThread");

        _host = new ScrobblingHost(trackInfoProvider);
        _host->moveToThread(&_thread);
        connect(&_thread, &QThread::finished, _host, &QObject::deleteLater);
        //connect(&_thread, &QThread::started, _host, &ScrobblingHost::load);

        _controller = new GlobalScrobblingController();
        connect(
            _controller, &GlobalScrobblingController::wakeUpRequested,
            _host, &ScrobblingHost::wakeUpForUser
        );
        connect(
            _controller, &GlobalScrobblingController::enableScrobblingRequested,
            _host, &ScrobblingHost::enableScrobbling
        );
        connect(
            _controller, &GlobalScrobblingController::nowPlayingUpdateRequested,
            _host, &ScrobblingHost::setNowPlayingTrack
        );

        _thread.start();
    }

    Scrobbling::~Scrobbling()
    {
        _thread.quit();
        _thread.wait();
    }

    GlobalScrobblingController* Scrobbling::getController()
    {
        return _controller;
    }

    UserScrobblingController* Scrobbling::getControllerForUser(uint userId)
    {
        auto controller = _userControllers.value(userId, nullptr);
        if (controller) return controller;

        controller = createUserController(userId);

        _userControllers.insert(userId, controller);
        return controller;
    }

    UserScrobblingController* Scrobbling::createUserController(uint userId)
    {
        auto controller = new UserScrobblingController(userId);
        controller->setParent(this);

        connect(
            controller, &UserScrobblingController::providerEnableOrDisableRequested,
            _host, &ScrobblingHost::setProviderEnabledForUser
        );

        connect(
            controller, &UserScrobblingController::wakeUpRequested,
            _host, &ScrobblingHost::wakeUpForUser
        );

        connect(
            controller, &UserScrobblingController::scrobblingProviderInfoRequested,
            _host, &ScrobblingHost::retrieveScrobblingProviderInfo
        );

        connect(
            _host, &ScrobblingHost::scrobblingProviderInfoSignal,
            controller,
            [controller, userId](uint eventUserId, ScrobblingProvider provider,
                                 bool enabled, ScrobblerStatus status)
            {
                if (eventUserId != userId) return; /* user does not match */

                Q_EMIT controller->scrobblingProviderInfo(provider, status, enabled);
            }
        );

        connect(
            _host, &ScrobblingHost::scrobblerStatusChanged,
            controller,
            [controller, userId](uint eventUserId, ScrobblingProvider provider,
                                 ScrobblerStatus status)
            {
                if (eventUserId != userId) return; /* user does not match */

                Q_EMIT controller->scrobblerStatusChanged(provider, status);
            }
        );

        connect(
            _host, &ScrobblingHost::scrobblingProviderEnabledChanged,
            controller,
            [controller, userId](uint eventUserId, ScrobblingProvider provider,
                                 bool enabled)
            {
                if (eventUserId != userId) return; /* user does not match */

                Q_EMIT controller->scrobblingProviderEnabledChanged(provider, enabled);
            }
        );

        return controller;
    }

    SimpleFuture<Result> Scrobbling::authenticateForProvider(uint userId,
                                                             ScrobblingProvider provider,
                                                             QString user,
                                                             QString password)
    {
        return
            Async::runOnEventLoop<Result>(
                _host,
                [host = _host, userId, provider, user, password]()
                {
                    return host->authenticateForProvider(userId, provider, user,
                                                         password);
                }
            );
    }
}
