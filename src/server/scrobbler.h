/*
    Copyright (C) 2018, Kevin Andre <hyperquantum@gmail.com>

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

#include "scrobblingbackend.h"

#include <QDateTime>
#include <QObject>
#include <QQueue>
#include <QString>
#include <QVector>

#include <memory>

QT_FORWARD_DECLARE_CLASS(QTimer)

namespace PMP {

    class TrackToScrobble {
    public:
        TrackToScrobble();
        virtual ~TrackToScrobble();

        virtual QDateTime timestamp() const = 0;
        virtual QString title() const = 0;
        virtual QString artist() const = 0;
        virtual QString album() const = 0;

        virtual void scrobbledSuccessfully() = 0;
        virtual void cannotBeScrobbled() = 0;
    };

    class ScrobblingDataProvider {
    public:
        virtual ~ScrobblingDataProvider();

        virtual QVector<std::shared_ptr<TrackToScrobble>> getNextTracksToScrobble() = 0;

    protected:
        ScrobblingDataProvider();
    };

    class Scrobbler : public QObject {
        Q_OBJECT
    public:
        Scrobbler(QObject* parent, ScrobblingDataProvider* dataProvider,
                  ScrobblingBackend* backend);

    public slots:
        void wakeUp();

    Q_SIGNALS:

    private slots:
        void timeoutTimerTimedOut();
        void gotScrobbleResult(ScrobbleResult result);
        void backendStateChanged(ScrobblingBackendState newState);

    private:
        void checkIfWeHaveSomethingToDo();
        void sendNextScrobble();

        ScrobblingDataProvider* _dataProvider;
        ScrobblingBackend* _backend;
        QQueue<std::shared_ptr<TrackToScrobble>> _tracksToScrobble;
        std::shared_ptr<TrackToScrobble> _pendingScrobble;
        QTimer* _timeoutTimer;
    };
}
#endif
