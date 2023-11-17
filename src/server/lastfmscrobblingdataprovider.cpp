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

#include "lastfmscrobblingdataprovider.h"

#include "database.h"

#include <QtDebug>
#include <QVector>

namespace PMP::Server
{
    class LastFmScrobblingDataProvider::LastFmTrackToScrobble : public TrackToScrobble
    {
    public:
        LastFmTrackToScrobble(LastFmScrobblingDataProvider* parent, quint32 id,
                              QDateTime timestamp, uint hashId)
         : _parent(parent),
            _id(id),
            _timestamp(timestamp),
            _hashId(hashId)
        {
            //
        }

        QDateTime timestamp() const override { return _timestamp; }
        uint hashId() const override { return _hashId; }

        void scrobbledSuccessfully() override;
        void scrobbleIgnored() override;

    private:
        LastFmScrobblingDataProvider* _parent;
        quint32 _id;
        QDateTime _timestamp;
        uint _hashId;
    };

    void LastFmScrobblingDataProvider::LastFmTrackToScrobble::scrobbledSuccessfully()
    {
        _parent->scrobbledSuccessfully(_id);
    }

    void LastFmScrobblingDataProvider::LastFmTrackToScrobble::scrobbleIgnored()
    {
        _parent->scrobbleIgnored(_id);
    }

    /* ============================================================================ */

    LastFmScrobblingDataProvider::LastFmScrobblingDataProvider(quint32 user)
     : _user(user),
        _scrobbledUpTo(0),
        _fetchedUpTo(0)
    {
        //
    }

    QVector<QSharedPointer<TrackToScrobble>>
        LastFmScrobblingDataProvider::getNextTracksToScrobble()
    {
        auto database = Database::getDatabaseForCurrentThread();
        if (!database)
            return {}; /* no database connection */

        if (_fetchedUpTo == 0)
        {
            bool ok;
            _scrobbledUpTo = database->getLastFmScrobbledUpTo(_user, &ok);
            _fetchedUpTo = _scrobbledUpTo;

            if (!ok) return {}; /* could not get the scrobbled-up-to value */
        }

        auto daysInPast = 60;
        auto earliestTime = QDateTime::currentDateTimeUtc().addDays(-daysInPast);
        auto fetchSize = 5;

        const auto history =
                database->getUserHistoryForScrobbling(_user, _fetchedUpTo + 1,
                                                      earliestTime, fetchSize);
        if (history.empty()) return {}; /* nothing more to scrobble */

        _fetchedUpTo = history.last().id;
        qDebug() << "LastFmScrobblingDataProvider: fetched tracks to scrobble up to"
                 << _fetchedUpTo;

        QVector<QSharedPointer<TrackToScrobble>> result;
        result.reserve(history.size());

        for (auto const& historyRecord : history)
        {
            auto track =
                QSharedPointer<LastFmTrackToScrobble>::create(
                    this, historyRecord.id, historyRecord.start, historyRecord.hashId
                );

            result << track;
        }

        return result;
    }

    void LastFmScrobblingDataProvider::scrobbledSuccessfully(quint32 id)
    {
        qDebug() << "marking history entry" << id
                 << "as successfully scrobbled to Last.fm  (PMP user ID" << _user << ")";

        _scrobbledUpTo = qMax(_scrobbledUpTo, id);
        updateScrobbledUpTo();
    }

    void LastFmScrobblingDataProvider::scrobbleIgnored(quint32 id)
    {
        qDebug() << "marking history entry" << id
                 << "as already scrobbled to Last.fm  (PMP user ID" << _user << ")";

        /* just like a successful scrobble */
        _scrobbledUpTo = qMax(_scrobbledUpTo, id);
        updateScrobbledUpTo();
    }

    void LastFmScrobblingDataProvider::updateScrobbledUpTo()
    {
        auto database = Database::getDatabaseForCurrentThread();
        if (!database)
        {
             /* no database connection */
            qWarning() << "could not update scrobbled-up-to: database unavailable";
            return;
        }

        if (!database->updateLastFmScrobbledUpTo(_user, _scrobbledUpTo))
        {
            qWarning() << "could not update scrobbled-up-to";
        }
    }
}
