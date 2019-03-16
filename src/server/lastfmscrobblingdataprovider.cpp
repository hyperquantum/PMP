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

#include "lastfmscrobblingdataprovider.h"

#include "database.h"
#include "resolver.h"

#include <QtDebug>
#include <QVector>

namespace PMP {

    class LastFmScrobblingDataProvider::TrackForScrobbling : public TrackToScrobble {
    public:
        TrackForScrobbling(LastFmScrobblingDataProvider* parent, quint32 id,
                           QDateTime timestamp, QString title, QString artist,
                           QString album)
         : _parent(parent), _id(id), _timestamp(timestamp), _title(title),
           _artist(artist), _album(album)
        {
            //
        }

        QDateTime timestamp() const { return _timestamp; }
        QString title() const { return _title; }
        QString artist() const { return _artist; }
        QString album() const { return _album; }

        void scrobbledSuccessfully();
        void scrobbleIgnored();

    private:
        LastFmScrobblingDataProvider* _parent;
        quint32 _id;
        QDateTime _timestamp;
        QString _title;
        QString _artist;
        QString _album;
    };

    void LastFmScrobblingDataProvider::TrackForScrobbling::scrobbledSuccessfully() {
        _parent->scrobbledSuccessfully(_id);
    }

    void LastFmScrobblingDataProvider::TrackForScrobbling::scrobbleIgnored() {
        _parent->scrobbleIgnored(_id);
    }

    /* ============================================================================ */

    LastFmScrobblingDataProvider::LastFmScrobblingDataProvider(quint32 user,
                                                               Resolver* resolver)
     : _resolver(resolver), _user(user), _scrobbledUpTo(0), _fetchedUpTo(0)
    {
        //
    }

    QVector<std::shared_ptr<TrackToScrobble>>
        LastFmScrobblingDataProvider::getNextTracksToScrobble()
    {
        auto database = Database::getDatabaseForCurrentThread();
        if (!database)
            return {}; /* no database connection */

        if (_fetchedUpTo == 0) {
            bool ok;
            _scrobbledUpTo = database->getLastFmScrobbledUpTo(_user, &ok);
            _fetchedUpTo = _scrobbledUpTo;

            if (!ok) return {}; /* could not get the scrobbled-up-to value */
        }

        auto daysInPast = 60;
        auto earliestTime = QDateTime::currentDateTimeUtc().addDays(-daysInPast);
        auto fetchSize = 5;

        auto history =
                database->getUserHistoryForScrobbling(_user, _fetchedUpTo + 1,
                                                      earliestTime, fetchSize);
        if (history.empty()) return {}; /* nothing more to scrobble */

        _fetchedUpTo = history.last().id;
        qDebug() << "fetched tracks to scrobble up to" << _fetchedUpTo;

        QVector<uint> hashIds;
        hashIds.reserve(history.size());
        Q_FOREACH(auto historyRecord, history) {
            hashIds << historyRecord.hashId;
        }

        QVector<std::shared_ptr<TrackToScrobble>> result;
        result.reserve(history.size());

        Q_FOREACH(auto historyRecord, history) {
            auto trackInfo = _resolver->getHashTrackInfo(historyRecord.hashId);

            /* it's possible that we don't have artist/title/album info yet/anymore, and
               in that case the track will not be scrobbled but simply ignored. We can't
               really fix that, because we can't pause scrobbling potentially forever
               while waiting for title/artist/album info on a track that we may never see
               again. */
            if (trackInfo.title().isEmpty() || trackInfo.artist().isEmpty()) {
                qDebug() << "cannot scrobble history record" << historyRecord.id
                         << "with hash ID" << historyRecord.hashId
                         << "because artist/title info is incomplete or missing";
                continue;
            }

            auto track =
                new TrackForScrobbling(
                    this, historyRecord.id, historyRecord.start,
                        trackInfo.title(), trackInfo.artist(), trackInfo.album()
                );

            result << std::shared_ptr<TrackToScrobble>(track);
        }

        return result;
    }

    void LastFmScrobblingDataProvider::scrobbledSuccessfully(quint32 id) {
        _scrobbledUpTo = qMax(_scrobbledUpTo, id);
        updateScrobbledUpTo();
    }

    void LastFmScrobblingDataProvider::scrobbleIgnored(quint32 id) {
        /* just like a successful scrobble */
        _scrobbledUpTo = qMax(_scrobbledUpTo, id);
        updateScrobbledUpTo();
    }

    void LastFmScrobblingDataProvider::updateScrobbledUpTo() {
        auto database = Database::getDatabaseForCurrentThread();
        if (!database) {
             /* no database connection */
            qWarning() << "could not update scrobbled-up-to: database unavailable";
            return;
        }

        if (!database->updateLastFmScrobbledUpTo(_user, _scrobbledUpTo)) {
            qWarning() << "could not update scrobbled-up-to";
        }
    }
}
