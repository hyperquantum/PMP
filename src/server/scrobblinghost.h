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

#ifndef PMP_SCROBBLINGHOST_H
#define PMP_SCROBBLINGHOST_H

#include "common/future.h"
#include "common/scrobblerstatus.h"
#include "common/scrobblingprovider.h"

#include "result.h"
#include "scrobblingtrack.h"

#include <QDateTime>
#include <QObject>
#include <QHash>
#include <QString>

#include <functional>

namespace PMP
{
    enum class ScrobblingProvider;
}

namespace PMP::Server
{
    class Database;
    class Scrobbler;
    enum class ScrobblingBackendState;
    class TrackInfoProvider;

    namespace DatabaseRecords
    {
        class LastFmScrobblingDataRecord;
        class UserScrobblingDataRecord;
    }

    class ScrobblingHost : public QObject
    {
        Q_OBJECT
    public:
        ScrobblingHost(TrackInfoProvider* trackInfoProvider);

        SimpleFuture<Result> authenticateForProvider(uint userId,
                                                     ScrobblingProvider provider,
                                                     QString user,
                                                     QString password);

    public Q_SLOTS:
        void enableScrobbling();
        void load();
        void wakeUpForUser(uint userId);
        void setProviderEnabledForUser(uint userId, PMP::ScrobblingProvider provider,
                                       bool enabled);
        void retrieveScrobblingProviderInfo(uint userId);

        //void setNowPlayingNothing(uint userId);
        void setNowPlayingTrack(uint userId, QDateTime startTime,
                                PMP::Server::ScrobblingTrack track);

    Q_SIGNALS:
        void scrobblingProviderInfoSignal(uint userId, PMP::ScrobblingProvider provider,
                                          bool enabled, PMP::ScrobblerStatus status);
        void scrobblerStatusChanged(uint userId, PMP::ScrobblingProvider provider,
                                    PMP::ScrobblerStatus status);
        void scrobblingProviderEnabledChanged(uint userId,
                                              PMP::ScrobblingProvider provider,
                                              bool enabled);

    private:
        struct ScrobblerData
        {
            ScrobblerData();

            bool enabled;
            ScrobblerStatus status;
            Scrobbler* scrobbler;
        };

        void doForAllProviders(std::function<void (ScrobblingProvider)> action);

        void ensureObfuscated(DatabaseRecords::UserScrobblingDataRecord& record,
                              Database& db);

        void loadScrobblers(DatabaseRecords::UserScrobblingDataRecord const& record);
        void loadScrobbler(DatabaseRecords::UserScrobblingDataRecord const& record,
                           ScrobblingProvider provider, bool enabled);
        void enableDisableScrobbler(uint userId, ScrobblingProvider provider,
                                    bool enabled);
        void createScrobblerIfNotExists(uint userId, ScrobblingProvider provider);
        void destroyScrobblerIfExists(uint userId, ScrobblingProvider provider);
        Scrobbler* createLastFmScrobbler(uint userId,
                                DatabaseRecords::LastFmScrobblingDataRecord const& data);
        void installScrobblerSignalHandlers(uint userId, ScrobblingProvider provider,
                                            Scrobbler* scrobbler);

        TrackInfoProvider* _trackInfoProvider;
        QHash<uint, QHash<ScrobblingProvider, ScrobblerData>> _scrobblersData;
        bool _hostEnabled;
    };
}
#endif
