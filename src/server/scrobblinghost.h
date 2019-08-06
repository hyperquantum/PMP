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

#ifndef PMP_SCROBBLINGHOST_H
#define PMP_SCROBBLINGHOST_H

#include "clientrequestorigin.h"

#include <QDateTime>
#include <QObject>
#include <QHash>
#include <QString>

namespace PMP {

    class LastFmScrobblingDataRecord;
    class Resolver;
    class Scrobbler;

    class ScrobblingHost : public QObject {
        Q_OBJECT
    public:
        ScrobblingHost(Resolver* resolver);

    public slots:
        void enableScrobbling();
        void load();
        void wakeUpForUser(uint userId);
        void setLastFmEnabledForUser(uint userId, bool enabled);

        //void setNowPlayingNothing(uint userId);
        void setNowPlayingTrack(uint userId, QDateTime startTime,
                                QString title, QString artist, QString album,
                                int trackDurationSeconds = -1);

    Q_SIGNALS:
        void needLastFmCredentials(uint userId, QString suggestedUsername);
        void gotLastFmAuthenticationResult(uint userId, ClientRequestOrigin origin,
                                           bool success);
        void errorOccurredDuringAuthentication(uint userId, ClientRequestOrigin origin);

    private:
        void createLastFmScrobbler(uint userId, LastFmScrobblingDataRecord const& data);
        QString decodeToken(QString token) const;

        Resolver* _resolver;
        QHash<uint, Scrobbler*> _scrobblers;
        bool _enabled;
    };

}
#endif
