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

#ifndef PMP_CLIENT_COLLECTIONWATCHER_H
#define PMP_CLIENT_COLLECTIONWATCHER_H

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

        virtual QHash<LocalHashId, CollectionTrackInfo> getCollection() = 0;
        virtual Nullable<CollectionTrackInfo> getTrackFromCache(LocalHashId hashId) = 0;
        virtual Future<CollectionTrackInfo, AnyResultMessageCode> getTrackInfo(
                                                                LocalHashId hashId) = 0;

    Q_SIGNALS:
        void downloadingInProgressChanged();
        void newTrackReceived(CollectionTrackInfo track);
        void trackAvailabilityChanged(LocalHashId hashId, bool isAvailable);
        void trackDataChanged(CollectionTrackInfo track);

    protected:
        explicit CollectionWatcher(QObject* parent = nullptr) : QObject(parent) {}
    };
}
#endif
