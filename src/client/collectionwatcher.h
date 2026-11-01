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

#ifndef PMP_CLIENT_COLLECTIONWATCHER_H
#define PMP_CLIENT_COLLECTIONWATCHER_H

#include "common/filehash.h"
#include "common/future.h"
#include "common/nullable.h"
#include "common/resultmessageerrorcode.h"

#include "collectiontrackinfo.h"

#include <QHash>
#include <QObject>

namespace PMP::Client
{
    class CollectionWatcher : public QObject
    {
        Q_OBJECT
    public:
        virtual ~CollectionWatcher() {}

        virtual bool isAlbumArtistSupported() const = 0;

        virtual void enableCollectionDownloading() = 0;
        virtual bool downloadingInProgress() const = 0;

        virtual QHash<LocalTrackId, CollectionTrackInfo> getCollection() = 0;

        virtual Future<LocalTrackId, AnyResultMessageCode> convertTrackHashToLocalId(
                                                                FileHash const& hash) = 0;
        virtual Future<FileHash, AnyResultMessageCode> convertLocalTrackIdToHash(
                                                                LocalTrackId localId) = 0;
        virtual Nullable<LocalTrackId> tryConvertTrackHashToLocalId(
                                                                FileHash const& hash) = 0;
        virtual Nullable<FileHash> tryConvertLocalTrackIdToHash(LocalTrackId localId) = 0;

        virtual Nullable<CollectionTrackInfo> getTrackFromCache(LocalTrackId hashId) = 0;
        virtual Future<CollectionTrackInfo, AnyResultMessageCode> getTrackInfo(
                                                                LocalTrackId hashId) = 0;
        virtual Future<CollectionTrackInfo, AnyResultMessageCode> getTrackInfo(
                                                                FileHash const& hash) = 0;

    Q_SIGNALS:
        void downloadingInProgressChanged();
        void newTrackReceived(CollectionTrackInfo track);
        void trackAvailabilityChanged(LocalTrackId hashId, bool isAvailable);
        void trackDataChanged(CollectionTrackInfo track);

    protected:
        explicit CollectionWatcher(QObject* parent = nullptr) : QObject(parent) {}
    };
}
#endif
