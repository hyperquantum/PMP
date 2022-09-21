/*
    Copyright (C) 2020-2022, Kevin Andre <hyperquantum@gmail.com>

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

#include "collectionwatcherimpl.h"

#include "collectionfetcher.h"
#include "serverconnection.h"

#include <QtDebug>

namespace PMP
{
    CollectionWatcherImpl::CollectionWatcherImpl(ServerConnection* connection)
     : CollectionWatcher(connection),
       _connection(connection),
       _autoDownload(false),
       _downloading(false)
    {
        connect(
            connection, &ServerConnection::collectionTracksAvailabilityChanged,
            this, &CollectionWatcherImpl::onCollectionTracksAvailabilityChanged
        );
        connect(
            connection, &ServerConnection::collectionTracksChanged,
            this, &CollectionWatcherImpl::onCollectionTracksChanged
        );

        if (_connection->isConnected())
            onConnected();
    }

    void CollectionWatcherImpl::enableCollectionDownloading()
    {
        if (_autoDownload)
            return; /* no action needed */

        _autoDownload = true;

        if (_connection->isConnected())
            startDownload();
    }

    QHash<FileHash, CollectionTrackInfo> CollectionWatcherImpl::getCollection()
    {
        return _collectionHash;
    }

    CollectionTrackInfo CollectionWatcherImpl::getTrack(const FileHash& hash)
    {
        auto it = _collectionHash.find(hash);

        if (it == _collectionHash.end())
            return CollectionTrackInfo();

        return it.value();
    }

    void CollectionWatcherImpl::onConnected()
    {
        if (_autoDownload)
            startDownload();
    }

    void CollectionWatcherImpl::onCollectionPartReceived(
                                                      QVector<CollectionTrackInfo> tracks)
    {
        qDebug() << "download: received part with" << tracks.size() << "tracks";

        for (auto const& track : tracks)
        {
            if (_collectionHash.contains(track.hash()))
                continue; /* don't update */

            _collectionHash.insert(track.hash(), track);
            Q_EMIT newTrackReceived(track);
        }
    }

    void CollectionWatcherImpl::onCollectionDownloadCompleted()
    {
        qDebug() << "collection download completed";
        _downloading = false;
    }

    void CollectionWatcherImpl::onCollectionDownloadError()
    {
        qWarning() << "collection download failed";
        _downloading = false;
    }

    void CollectionWatcherImpl::onCollectionTracksAvailabilityChanged(
                                                            QVector<FileHash> available,
                                                            QVector<FileHash> unavailable)
    {
        updateTrackAvailability(available, true);
        updateTrackAvailability(unavailable, false);
    }

    void CollectionWatcherImpl::onCollectionTracksChanged(
                                                     QVector<CollectionTrackInfo> changes)
    {
        for (auto const& track : changes)
        {
            updateTrackData(track);
        }
    }

    void CollectionWatcherImpl::startDownload()
    {
        if (_downloading) return;

        qDebug() << "starting collection download";
        auto fetcher = new CollectionFetcher();

        connect(
            fetcher, &CollectionFetcher::receivedData,
            this, &CollectionWatcherImpl::onCollectionPartReceived
        );
        connect(
            fetcher, &CollectionFetcher::completed,
            this, &CollectionWatcherImpl::onCollectionDownloadCompleted
        );
        connect(
            fetcher, &CollectionFetcher::errorOccurred,
            this, &CollectionWatcherImpl::onCollectionDownloadError
        );

        _connection->fetchCollection(fetcher);
        _downloading = true;
    }

    void CollectionWatcherImpl::updateTrackAvailability(QVector<FileHash> hashes,
                                                        bool available)
    {
        for (auto const& hash : hashes)
        {
            auto it = _collectionHash.find(hash);

            if (it != _collectionHash.end())
            {
                if (it.value().isAvailable() != available)
                {
                    it.value().setAvailable(available);
                    Q_EMIT trackAvailabilityChanged(hash, available);
                }

                continue;
            }

            /* it seems we received availability info for a track still unknown to us */

            if (!available)
                continue; /* the track is not available, so we don't have to care */

            qWarning() << "received positive track availability for an unknown track; "
                       << "inserting placeholder data for hash" << hash;

            /* add the track without its title, artist, etc. */
            CollectionTrackInfo track(hash, available);
            _collectionHash.insert(hash, track);

            Q_EMIT newTrackReceived(track);
        }
    }

    void CollectionWatcherImpl::updateTrackData(const CollectionTrackInfo& track)
    {
        auto it = _collectionHash.find(track.hash());

        if (it == _collectionHash.end()) /* the track is unknown to us */
        {
            _collectionHash.insert(track.hash(), track);
            Q_EMIT newTrackReceived(track);
            return;
        }

        if (it.value() == track)
            return; /* no difference found */

        it.value() = track;
        Q_EMIT trackDataChanged(track);
    }
}
