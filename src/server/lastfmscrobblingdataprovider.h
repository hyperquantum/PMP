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

#ifndef PMP_LASTFMSCROBBLINGDATAPROVIDER_H
#define PMP_LASTFMSCROBBLINGDATAPROVIDER_H

#include "scrobblingdataprovider.h"

#include <QtGlobal>

namespace PMP {

    class Resolver;

    class LastFmScrobblingDataProvider : public ScrobblingDataProvider {
    public:
        LastFmScrobblingDataProvider(quint32 user, Resolver* resolver);

        QVector<std::shared_ptr<TrackToScrobble>> getNextTracksToScrobble() override;

    private:
        class TrackForScrobbling;

        void scrobbledSuccessfully(quint32 id);
        void scrobbleIgnored(quint32 id);
        void updateScrobbledUpTo();

        Resolver* _resolver;
        quint32 _user;
        quint32 _scrobbledUpTo;
        quint32 _fetchedUpTo;
    };

}
#endif
