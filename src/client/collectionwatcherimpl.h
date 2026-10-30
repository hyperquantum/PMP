/*
    Copyright (C) 2020-2026, Kevin André <hyperquantum@gmail.com>

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

#ifndef PMP_CLIENT_COLLECTIONWATCHERIMPL_H
#define PMP_CLIENT_COLLECTIONWATCHERIMPL_H

#include "collectionwatcher.h"

#include <QHash>
#include <QVector>

namespace PMP::Client
{
    class ServerConnection;

    class CollectionWatcherImpl : public CollectionWatcher
    {
        Q_OBJECT
    public:
        explicit CollectionWatcherImpl(ServerConnection* connection);

        bool isAlbumArtistSupported() const override;

        void enableCollectionDownloading() override;
        bool downloadingInProgress() const override;

        QHash<LocalTrackId, CollectionTrackInfo> getCollection() override;

        Future<LocalTrackId, AnyResultMessageCode> convertTrackHashToLocalId(
                                                        FileHash const& hash) override;
        Future<FileHash, AnyResultMessageCode> convertLocalTrackIdToHash(
            LocalTrackId localId) override;
        Nullable<LocalTrackId> tryConvertTrackHashToLocalId(
            FileHash const& hash) override;
        Nullable<FileHash> tryConvertLocalTrackIdToHash(LocalTrackId localId) override;

        Nullable<CollectionTrackInfo> getTrackFromCache(LocalTrackId hashId) override;
        Future<CollectionTrackInfo, AnyResultMessageCode> getTrackInfo(
                                                            LocalTrackId hashId) override;
        Future<CollectionTrackInfo, AnyResultMessageCode> getTrackInfo(
                                                        FileHash const& hash) override;

    private Q_SLOTS:
        void onConnected();
        void onCollectionPartReceived(QVector<CollectionTrackInfo> tracks);
        void onCollectionDownloadCompleted();
        void onCollectionDownloadError();
        void onCollectionTracksAvailabilityChanged(
                                        QVector<PMP::Client::LocalTrackId> available,
                                        QVector<PMP::Client::LocalTrackId> unavailable);
        void onCollectionTracksChanged(QVector<PMP::Client::CollectionTrackInfo> changes);

    private:
        Future<CollectionTrackInfo, AnyResultMessageCode> fetchTrackInfoFromServer(
                                                                    LocalTrackId hashId);
        Future<CollectionTrackInfo, AnyResultMessageCode> fetchTrackInfoFromServer(
                                                                    FileHash const& hash);
        void startDownload();
        void updateTrackAvailability(QVector<LocalTrackId> hashes, bool available);
        void updateTrackData(CollectionTrackInfo const& track);

        ServerConnection* _connection;
        QHash<LocalTrackId, CollectionTrackInfo> _collectionHash;
        bool _autoDownload;
        bool _downloading;
    };
}
#endif
