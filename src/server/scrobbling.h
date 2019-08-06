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

#ifndef PMP_SCROBBLING_H
#define PMP_SCROBBLING_H

#include <QDateTime>
#include <QHash>
#include <QObject>
#include <QThread>

namespace PMP {

    class Resolver;
    class ScrobblingHost;

    class ScrobblingController : public QObject {
        Q_OBJECT
    public:
        ScrobblingController();

    public slots:
        void enableScrobbling();
        void wakeUp(uint userId);
        void updateNowPlaying(uint userId, QDateTime startTime,
                              QString title, QString artist, QString album,
                              int trackDurationSeconds = -1);

    Q_SIGNALS:
        void enableScrobblingRequested();
        void wakeUpRequested(uint userId);

        void nowPlayingUpdateRequested(uint userId, QDateTime startTime,
                                       QString title, QString artist, QString album,
                                       int trackDurationSeconds = -1);
    };

    class UserScrobblingController : public QObject {
        Q_OBJECT
    public:
        UserScrobblingController(uint userId);

    public slots:
        void wakeUp();
        void enableLastFm();

    Q_SIGNALS:
        void wakeUpRequested(uint userId);
        void lastFmEnabledChanged(uint userId, bool enabled);

    private:
        uint _userId;
    };

    class Scrobbling : public QObject {
        Q_OBJECT
    public:
        explicit Scrobbling(QObject* parent, Resolver* resolver);
        ~Scrobbling();

        ScrobblingController* getController();
        UserScrobblingController* getControllerForUser(uint userId);

    Q_SIGNALS:

    private:
        Resolver* _resolver;
        ScrobblingHost* _host;
        QThread _thread;
        ScrobblingController* _controller;
        QHash<uint, UserScrobblingController*> _userControllers;
    };
}
#endif
