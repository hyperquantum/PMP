/*
    Copyright (C) 2024-2025, Kevin André <hyperquantum@gmail.com>

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

#ifndef PMP_SEARCHING_H
#define PMP_SEARCHING_H

#include "client/collectiontrackinfo.h"
#include "client/localhashid.h"
#include "client/searchquery.h"

#include <QHash>
#include <QList>
#include <QObject>
#include <QStringList>

namespace PMP::Client
{
    class CollectionWatcher;
}

namespace PMP
{
    class SearchData;

    class SearchData : public QObject
    {
        Q_OBJECT
    public:
        SearchData(QObject* parent, Client::CollectionWatcher* collectionWatcher);

        bool isFileMatchForQuery(Client::LocalHashId hashId,
                                 Client::SearchQuery const& query) const;

        QList<Client::LocalHashId> getAllMatchesForQuery(
                                                Client::SearchQuery const& query) const;

    private Q_SLOTS:
        void onNewTrackReceived(Client::CollectionTrackInfo track);
        void onTrackDataChanged(Client::CollectionTrackInfo track);

    private:
        void updateTrackData(Client::CollectionTrackInfo const& track);

        struct TrackSearchStrings
        {
            QString title;
            QString artist;
            QString album;
            QString albumArtist;
        };

        bool isTrackDataMatchForQuery(TrackSearchStrings const& trackData,
                                      Client::SearchQuery const& query) const;

        QHash<Client::LocalHashId, TrackSearchStrings> _trackData;
    };
}
#endif
