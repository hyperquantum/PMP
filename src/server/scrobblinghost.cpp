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

    ScrobblingHost::ScrobblerData::ScrobblerData()
     : enabled(false), status(ScrobblerStatus::Unknown), scrobbler(nullptr)
    {
        //
    }

    /* ============================================================================ */

    ScrobblingHost::ScrobblingHost(Resolver* resolver)
     : _resolver(resolver), _hostEnabled(false)
    {
        //
    }

    void ScrobblingHost::enableScrobbling() {
        if (_hostEnabled)
            return; /* already enabled */

        qDebug() << "ScrobblingHost now enabled";
        _hostEnabled = true;
        load();
    }

    void ScrobblingHost::load() {
        if (!_hostEnabled) {
            qWarning() << "host not enabled, not going to load anything";
            return;
        }

        qDebug() << "(re)loading scrobbling settings for all users";

        auto db = Database::getDatabaseForCurrentThread();
        if (!db) {
            /* can't load */
            return;
        }

        auto userData = db->getUsersScrobblingData();

        for(auto& data : userData) {
            loadScrobblers(data);
        }
    }

    void ScrobblingHost::wakeUpForUser(uint userId) {
        if (!_hostEnabled) {
            qWarning() << "host not enabled, not going to wake up";
            return;
        }

        auto action =
            [this, userId](ScrobblingProvider provider) {
                auto scrobbler = _scrobblersData[userId][provider].scrobbler;

                /* wake up with a delay, to give the database enough time to store the
                   track that has just finished playing */
                if (scrobbler)
                    QTimer::singleShot(100, scrobbler, &Scrobbler::wakeUp);
            };

        doForAllProviders(action);
    }

    void ScrobblingHost::setProviderEnabledForUser(uint userId,
                                                   ScrobblingProvider provider,
                                                   bool enabled)
    {
        if (!_hostEnabled) {
            qWarning() << "host not enabled, not touching the provider";
            return;
        }

        auto db = Database::getDatabaseForCurrentThread();
        if (!db) { return; }

        switch (provider) {
            case ScrobblingProvider::LastFm:
                db->setLastFmScrobblingEnabled(userId, enabled);
                break;
            case ScrobblingProvider::Unknown:
            //default:
                return;
        }

        auto& data = _scrobblersData[userId][provider];
        auto enabledChanged = data.enabled != enabled;
        data.enabled = enabled;

        enableDisableScrobbler(userId, provider, enabled);

        if (enabledChanged)
            emit scrobblingProviderEnabledChanged(userId, provider, enabled);
    }

    void ScrobblingHost::retrieveScrobblingProviderInfo(uint userId) {
        if (!_hostEnabled) {
            qWarning() << "host not enabled, not sending provider info";
            return;
        }

        qDebug() << "going to emit scrobblingProviderInfoSignal signal(s)";

        /* Last.FM info; no other providers available yet */
        auto provider = ScrobblingProvider::LastFm;
        auto& data = _scrobblersData[userId][ScrobblingProvider::LastFm];
        if (data.enabled) {
            emit scrobblingProviderInfoSignal(userId, provider, true, data.status);
        }
        else {
            emit scrobblingProviderInfoSignal(userId, provider, false,
                                              ScrobblerStatus::Unknown);
        }
    }

    /*void ScrobblingHost::setNowPlayingNothing(uint userId) {
        auto scrobbler = _scrobblers.value(userId, nullptr);
        if (!scrobbler) return;

        scrobbler->nowPlayingNothing();
    }*/

    void ScrobblingHost::setNowPlayingTrack(uint userId, QDateTime startTime,
                                            QString title, QString artist, QString album,
                                            int trackDurationSeconds)
    {
        if (!_hostEnabled)
            return;

        auto action =
            [=](ScrobblingProvider provider) {
                auto scrobbler = _scrobblersData[userId][provider].scrobbler;
                if (!scrobbler) return;

                scrobbler->nowPlayingTrack(startTime, title, artist, album,
                                           trackDurationSeconds);
            };

        doForAllProviders(action);
    }

    void ScrobblingHost::loadScrobblers(UserScrobblingDataRecord const& record) {
        /* for all providers (currently only Last.FM) ... */
        loadScrobbler(record, ScrobblingProvider::LastFm, record.enableLastFmScrobbling);
    }

    void ScrobblingHost::loadScrobbler(UserScrobblingDataRecord const& record,
                                       ScrobblingProvider provider, bool enabled)
    {
        auto& data = _scrobblersData[record.userId][provider];
        data.enabled = enabled;

        enableDisableScrobbler(record.userId, provider, data.enabled);
    }

    void ScrobblingHost::enableDisableScrobbler(uint userId, ScrobblingProvider provider,
                                                bool enabled)
    {
        if (enabled) {
            createScrobblerIfNotExists(userId, provider);
        }
        else {
            destroyScrobblerIfExists(userId, provider);
        }
    }

    void ScrobblingHost::createScrobblerIfNotExists(uint userId,
                                                    ScrobblingProvider provider)
    {
        auto& data = _scrobblersData[userId][provider];
        if (data.scrobbler) return; /* already exists */

        auto db = Database::getDatabaseForCurrentThread();
        if (!db) { return; }

        switch (provider) {
            case ScrobblingProvider::LastFm:
            {
                auto lastFmData = db->getUserLastFmScrobblingData(userId);
                data.status = ScrobblerStatus::Unknown;
                data.scrobbler = createLastFmScrobbler(userId, lastFmData);
            }
                break;

            case ScrobblingProvider::Unknown:
                qWarning() << "cannot create 'Unknown' scrobbling provider";
                break;
        }

        auto scrobbler = data.scrobbler;

        if (!scrobbler)
        {
            qWarning() << "failed to create scrobbler for user" << userId
                       << "and provider" << provider;
            return;
        }

        installScrobblerSignalHandlers(userId, provider, scrobbler);
        scrobbler->wakeUp();
    }

    void ScrobblingHost::destroyScrobblerIfExists(uint userId,
                                                  ScrobblingProvider provider)
    {
        auto& data = _scrobblersData[userId][provider];
        auto scrobbler = data.scrobbler;
        if (!scrobbler) return; /* does not exist */

        data.scrobbler = nullptr;
        data.status = ScrobblerStatus::Unknown;
        scrobbler->deleteLater();
    }

    Scrobbler* ScrobblingHost::createLastFmScrobbler(uint userId,
                                                   LastFmScrobblingDataRecord const& data)
    {
        qDebug() << "creating Last.FM scrobbler for user with ID" << userId;

        auto dataProvider = new LastFmScrobblingDataProvider(userId, _resolver);
        auto lastFmBackend = new LastFmScrobblingBackend();

        connect(
            lastFmBackend, &LastFmScrobblingBackend::gotAuthenticationResult,
            this,
            [userId, this](bool success, ClientRequestOrigin origin) {
                emit this->gotAuthenticationResult(userId, ScrobblingProvider::LastFm,
                                                   origin, success);
            }
        );
        connect(
            lastFmBackend, &LastFmScrobblingBackend::errorOccurredDuringAuthentication,
            this,
            [userId, this](ClientRequestOrigin origin) {
                emit this->errorOccurredDuringAuthentication(userId, origin);
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
        return scrobbler;
    }

    void ScrobblingHost::installScrobblerSignalHandlers(uint userId,
                                                        ScrobblingProvider provider,
                                                        Scrobbler* scrobbler)
    {
        connect(
            scrobbler, &Scrobbler::statusChanged,
            this,
            [this, userId, provider](ScrobblerStatus status) {
                auto& data = _scrobblersData[userId][provider];
                if (data.status == status) return;

                qDebug() << "status changing from" << data.status << "to" << status
                         << "for user" << userId << "and provider" << provider;

                data.status = status;
                emit scrobblerStatusChanged(userId, provider, status);
            }
        );
    }

    QString ScrobblingHost::decodeToken(QString token) const {
        if (token.isEmpty()) return QString();

        if (!token.startsWith('?'))
            return token; /* token in plain text */

        // TODO : decode token
        qWarning() << "decoding of encoded token not implemented yet";

        return QString();
    }

    void ScrobblingHost::doForAllProviders(
                                          std::function<void (ScrobblingProvider)> action)
    {
        action(ScrobblingProvider::LastFm);
    }

}
