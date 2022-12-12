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

#ifndef PMP_COLLECTIONWATCHERIMPL_H
#define PMP_COLLECTIONWATCHERIMPL_H

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

        void enableCollectionDownloading() override;
        bool downloadingInProgress() const override;

        QHash<FileHash, CollectionTrackInfo> getCollection() override;
        CollectionTrackInfo getTrack(FileHash const& hash) override;

    private Q_SLOTS:
        void onConnected();
        void onCollectionPartReceived(QVector<CollectionTrackInfo> tracks);
        void onCollectionDownloadCompleted();
        void onCollectionDownloadError();
        void onCollectionTracksAvailabilityChanged(QVector<PMP::FileHash> available,
                                                   QVector<PMP::FileHash> unavailable);
        void onCollectionTracksChanged(QVector<PMP::CollectionTrackInfo> changes);

    private:
        void startDownload();
        void updateTrackAvailability(QVector<FileHash> hashes, bool available);
        void updateTrackData(CollectionTrackInfo const& track);

        ServerConnection* _connection;
        QHash<FileHash, CollectionTrackInfo> _collectionHash;
        bool _autoDownload;
        bool _downloading;
    };
}
#endif
