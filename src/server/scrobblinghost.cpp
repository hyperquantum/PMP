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

#include "scrobblinghost.h"

#include "database.h"
#include "lastfmscrobblingbackend.h"
#include "lastfmscrobblingdataprovider.h"
#include "scrobbler.h"

#include <QtDebug>
#include <QTimer>

namespace PMP {

    ScrobblingHost::ScrobblingHost(Resolver* resolver)
     : _resolver(resolver), _enabled(false)
    {
        //
    }

    void ScrobblingHost::enableScrobbling() {
        if (_enabled)
            return; /* already enabled */

        qDebug() << "ScrobblingHost now enabled";
        _enabled = true;
        load();
    }

    void ScrobblingHost::load() {
        if (!_enabled)
            return;

        qDebug() << "loading scrobbling settings for all users";

        auto db = Database::getDatabaseForCurrentThread();
        if (!db) {
            /* can't load */
            return;
        }

        auto userData = db->getUsersScrobblingData();

        for(auto& data : userData) {
            if (!data.enableLastFmScrobbling) continue;

            auto scrobbler = _scrobblers.value(data.userId, nullptr);

            if (!data.enableLastFmScrobbling) {
                // TODO : turn off
                //if (scrobbler) { }
            }
            else if (!scrobbler) {
                createLastFmScrobbler(data.userId, data);
            }
        }
    }

    void ScrobblingHost::wakeUpForUser(uint userId) {
        auto scrobbler = _scrobblers.value(userId, nullptr);

        /* wake up with a delay, to give the database enough time to store the track that
           has just finished playing */
        if (scrobbler)
            QTimer::singleShot(100, scrobbler, &Scrobbler::wakeUp);
    }

    void ScrobblingHost::setLastFmEnabledForUser(uint userId, bool enabled) {
        auto db = Database::getDatabaseForCurrentThread();
        if (!db) { return; }

        db->setLastFmScrobblingEnabled(userId, enabled);

        auto scrobbler = _scrobblers.value(userId, nullptr);

        if (!enabled) {
            // TODO

            return;
        }

        if (!scrobbler) {
            auto lastFmData = db->getUserLastFmScrobblingData(userId);
            createLastFmScrobbler(userId, lastFmData);
        }
    }

    void ScrobblingHost::createLastFmScrobbler(uint userId,
                                               LastFmScrobblingDataRecord const& data)
    {
        qDebug() << "creating Last.FM scrobbler for user with ID" << userId;

        auto dataProvider = new LastFmScrobblingDataProvider(userId, _resolver);
        auto lastFmBackend = new LastFmScrobblingBackend();

        connect(
            lastFmBackend, &LastFmScrobblingBackend::needUserCredentials,
            this,
            [userId, this](QString suggestedUserName, bool authenticationFailed) {
                emit this->needLastFmCredentials(userId, suggestedUserName,
                                                 authenticationFailed);
            }
        );

        if (!data.lastFmUser.isEmpty()) {
            lastFmBackend->setUsername(data.lastFmUser);
        }

        auto sessionKey = decodeToken(data.lastFmSessionKey);
        if (!sessionKey.isEmpty()) {
            lastFmBackend->setSessionKey(sessionKey);
        }

        auto scrobbler = new Scrobbler(this, dataProvider, lastFmBackend);
        _scrobblers.insert(userId, scrobbler);
        //scrobbler->wakeUp();
    }

    QString ScrobblingHost::decodeToken(QString token) const {
        if (token.isEmpty()) return QString();

        // TODO

        return token;
    }
}
