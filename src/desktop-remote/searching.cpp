/*
    Copyright (C) 2024, Kevin André <hyperquantum@gmail.com>

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

#include "searching.h"

#include "common/searchutil.h"

#include "client/collectionwatcher.h"

#include <QtDebug>

using namespace PMP::Client;

namespace PMP
{
    SearchQuery::SearchQuery()
    {
        //
    }

    SearchQuery::SearchQuery(const QString& query)
    {
        auto simplifiedSearchString = SearchUtil::toSearchString(query);

        _searchParts = simplifiedSearchString.split(QChar(' '), Qt::SkipEmptyParts);
    }

    void SearchQuery::clear()
    {
        _searchParts.clear();
    }

    bool SearchQuery::isEmpty() const
    {
        return _searchParts.isEmpty();
    }

    // =================================================================== //

    SearchData::SearchData(QObject* parent, CollectionWatcher* collectionWatcher)
        : QObject(parent)
    {
        qDebug() << "SearchData: running constructor";

        _trackData.reserve(500);

        connect(
            collectionWatcher, &CollectionWatcher::newTrackReceived,
            this, &SearchData::onNewTrackReceived
        );
        connect(
            collectionWatcher, &CollectionWatcher::trackDataChanged,
            this, &SearchData::onTrackDataChanged
        );
    }

    bool SearchData::isFileMatchForQuery(LocalHashId hashId,
                                         SearchQuery const& query) const
    {
        auto it = _trackData.constFind(hashId);
        if (it == _trackData.constEnd())
        {
            return false;
        }

        auto const& trackData = it.value();

        /* each part of the search query must be found in the track data */
        for (QString const& searchPart : query._searchParts)
        {
            /* we can do case sensitive comparisons here, because all text has already
               been changed to lowercase in advance */

            if (!trackData.title.contains(searchPart, Qt::CaseSensitive)
                && !trackData.artist.contains(searchPart, Qt::CaseSensitive)
                && !trackData.album.contains(searchPart, Qt::CaseSensitive)
                && !trackData.albumArtist.contains(searchPart, Qt::CaseSensitive))
            {
                return false; // no match
            }
        }

        return true; // match
    }

    void SearchData::onNewTrackReceived(CollectionTrackInfo track)
    {
        updateTrackData(track);
    }

    void SearchData::onTrackDataChanged(CollectionTrackInfo track)
    {
        updateTrackData(track);
    }

    void SearchData::updateTrackData(CollectionTrackInfo const& track)
    {
        auto& trackData = _trackData[track.hashId()];

        trackData.title = SearchUtil::toSearchString(track.title());
        trackData.artist = SearchUtil::toSearchString(track.artist());
        trackData.album = SearchUtil::toSearchString(track.album());
        trackData.albumArtist = SearchUtil::toSearchString(track.albumArtist());
    }
}
