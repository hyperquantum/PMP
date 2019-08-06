/*
    Copyright (C) 2019, Kevin Andre <hyperquantum@gmail.com>

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

#include "scrobblinghost.h"

#include <QtDebug>

namespace PMP {

    ScrobblingController::ScrobblingController()
    {
        //
    }

    void ScrobblingController::enableScrobbling() {
        emit enableScrobblingRequested();
    }

    void ScrobblingController::wakeUp(uint userId) {
        emit wakeUpRequested(userId);
    }

    void ScrobblingController::updateNowPlaying(uint userId, QDateTime startTime,
                                                QString title, QString artist,
                                                QString album,
                                                int trackDurationSeconds)
    {
        emit nowPlayingUpdateRequested(userId, startTime, title, artist, album,
                                       trackDurationSeconds);
    }

    /* ============================================================================ */

    UserScrobblingController::UserScrobblingController(uint userId)
     : _userId(userId)
    {
        //
    }

    void UserScrobblingController::wakeUp() {
        qDebug() << "UserScrobblingController::wakeUp called for user" << _userId;
        emit wakeUpRequested(_userId);
    }

    void UserScrobblingController::enableLastFm() {
        qDebug() << "UserScrobblingController::enableLastFm called for user" << _userId;
        emit lastFmEnabledChanged(_userId, true);
    }

    /* ============================================================================ */

    Scrobbling::Scrobbling(QObject *parent, Resolver* resolver)
        : QObject(parent), _resolver(resolver), _host(nullptr), _controller(nullptr)
    {
        _thread.setObjectName("ScrobblingThread");

        _host = new ScrobblingHost(resolver);
        _host->moveToThread(&_thread);
        connect(&_thread, &QThread::finished, _host, &QObject::deleteLater);
        //connect(&_thread, &QThread::started, _host, &ScrobblingHost::load);

        _controller = new ScrobblingController();
        connect(
            _controller, &ScrobblingController::wakeUpRequested,
            _host, &ScrobblingHost::wakeUpForUser
        );
        connect(
            _controller, &ScrobblingController::enableScrobblingRequested,
            _host, &ScrobblingHost::enableScrobbling
        );
        connect(
            _controller, &ScrobblingController::nowPlayingUpdateRequested,
            _host, &ScrobblingHost::setNowPlayingTrack
        );

        _thread.start();
    }

    Scrobbling::~Scrobbling() {
        _thread.quit();
        _thread.wait();
    }

    ScrobblingController* Scrobbling::getController() {
        return _controller;
    }

    UserScrobblingController* Scrobbling::getControllerForUser(uint userId) {
        auto controller = _userControllers.value(userId, nullptr);
        if (controller) return controller;

        controller = new UserScrobblingController(userId);
        controller->setParent(this);

        connect(
            controller, &UserScrobblingController::lastFmEnabledChanged,
            _host, &ScrobblingHost::setLastFmEnabledForUser
        );

        connect(
            controller, &UserScrobblingController::wakeUpRequested,
            _host, &ScrobblingHost::wakeUpForUser
        );

        _userControllers.insert(userId, controller);
        return controller;
    }
}
