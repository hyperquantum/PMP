/*
    Copyright (C) 2020-2024, Kevin Andre <hyperquantum@gmail.com>

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
#include "localhashidrepository.h"
#include "servercapabilities.h"
#include "serverconnection.h"

#include <QtDebug>

namespace PMP::Client
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

    bool CollectionWatcherImpl::isAlbumArtistSupported() const
    {
        return _connection->serverCapabilities().supportsAlbumArtist();
    }

    void CollectionWatcherImpl::enableCollectionDownloading()
    {
        if (_autoDownload)
            return; /* no action needed */

        _autoDownload = true;

        if (_connection->isConnected())
            startDownload();
    }

    bool CollectionWatcherImpl::downloadingInProgress() const
    {
        return _downloading;
    }

    QHash<LocalHashId, CollectionTrackInfo> CollectionWatcherImpl::getCollection()
    {
        return _collectionHash;
    }

    Nullable<CollectionTrackInfo> CollectionWatcherImpl::getTrackFromCache(
                                                                       LocalHashId hashId)
    {
        auto it = _collectionHash.find(hashId);

        if (it == _collectionHash.end())
        {
            /* download the data if possible */
            if (_connection->serverCapabilities().supportsRequestingIndividualTrackInfo())
                (void)getTrackInfoInternal(hashId);

            return null;
        }

        return it.value();
    }

    NewFuture<CollectionTrackInfo, AnyResultMessageCode>
        CollectionWatcherImpl::getTrackInfo(LocalHashId hashId)
    {
        auto it = _collectionHash.find(hashId);

        if (it != _collectionHash.end())
            return FutureResult(it.value());

        return getTrackInfoInternal(hashId);
    }

    NewFuture<CollectionTrackInfo, AnyResultMessageCode>
        CollectionWatcherImpl::getTrackInfo(const FileHash& hash)
    {
        auto hashId = _connection->hashIdRepository()->getId(hash);

        if (!hashId.isZero())
        {
            auto it = _collectionHash.find(hashId);

            if (it != _collectionHash.end())
                return FutureResult(it.value());
        }

        return getTrackInfoInternal(hash);
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
            if (_collectionHash.contains(track.hashId()))
                continue; /* don't update */

            _collectionHash.insert(track.hashId(), track);
            Q_EMIT newTrackReceived(track);
        }
    }

    void CollectionWatcherImpl::onCollectionDownloadCompleted()
    {
        qDebug() << "collection download completed";
        _downloading = false;
        Q_EMIT downloadingInProgressChanged();
    }

    void CollectionWatcherImpl::onCollectionDownloadError()
    {
        qWarning() << "collection download failed";
        _downloading = false;
        Q_EMIT downloadingInProgressChanged();
    }

    void CollectionWatcherImpl::onCollectionTracksAvailabilityChanged(
                                                         QVector<LocalHashId> available,
                                                         QVector<LocalHashId> unavailable)
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

    NewFuture<CollectionTrackInfo, AnyResultMessageCode>
        CollectionWatcherImpl::getTrackInfoInternal(LocalHashId hashId)
    {
        auto future = _connection->getTrackInfo(hashId);

        future.handleOnEventLoop(
            this,
            [this](ResultOrError<CollectionTrackInfo, AnyResultMessageCode> outcome)
            {
                if (outcome.succeeded())
                    updateTrackData(outcome.result());
            }
        );

        return future;
    }

    NewFuture<CollectionTrackInfo, AnyResultMessageCode>
        CollectionWatcherImpl::getTrackInfoInternal(const FileHash& hash)
    {
        auto future = _connection->getTrackInfo(hash);

        future.handleOnEventLoop(
            this,
            [this](ResultOrError<CollectionTrackInfo, AnyResultMessageCode> outcome)
            {
                if (outcome.succeeded())
                    updateTrackData(outcome.result());
            }
        );

        return future;
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

        Q_EMIT downloadingInProgressChanged();
    }

    void CollectionWatcherImpl::updateTrackAvailability(QVector<LocalHashId> hashes,
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
        auto it = _collectionHash.find(track.hashId());

        if (it == _collectionHash.end()) /* the track is unknown to us */
        {
            _collectionHash.insert(track.hashId(), track);
            Q_EMIT newTrackReceived(track);
            return;
        }

        if (it.value() == track)
            return; /* no difference found */

        it.value() = track;
        Q_EMIT trackDataChanged(track);
    }
}
