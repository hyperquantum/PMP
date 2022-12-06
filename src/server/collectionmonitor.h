/*
    Copyright (C) 2015-2022, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_COLLECTIONMONITOR_H
#define PMP_COLLECTIONMONITOR_H

#include "common/collectiontrackinfo.h"

#include <QHash>
#include <QMetaType>
#include <QObject>
#include <QPair>
#include <QVector>

namespace PMP::Server
{
    /*! class that monitors the collection on behalf of connected remotes */
    class CollectionMonitor : public QObject
    {
        Q_OBJECT
    public:
        CollectionMonitor(QObject* parent = 0);

    public Q_SLOTS:
        void hashBecameAvailable(PMP::FileHash hash);
        void hashBecameUnavailable(PMP::FileHash hash);
        void hashTagInfoChanged(PMP::FileHash hash, QString title, QString artist,
                                QString album, qint32 lengthInMilliseconds);

    Q_SIGNALS:
        void hashAvailabilityChanged(QVector<PMP::FileHash> available,
                                     QVector<PMP::FileHash> unavailable);
        void hashInfoChanged(QVector<PMP::CollectionTrackInfo> changes);

    private Q_SLOTS:
        void emitNotifications();

    private:
        void checkNeedToSendNotifications();
        void emitFullNotifications(QVector<FileHash> hashes);
        void emitAvailabilityNotifications(QVector<FileHash> hashes);

        struct HashInfo {
            bool isAvailable;
            QString title, artist, album;
            qint32 lengthInMilliseconds;

            HashInfo()
             : isAvailable(false), lengthInMilliseconds(-1)
            {
                //
            }
        };

        struct Changed {
            bool availability;
            bool tags;

            Changed()
             : availability(false), tags(false)
            {
                //
            }
        };

        QHash<FileHash, HashInfo> _collection;
        QHash<FileHash, Changed> _pendingNotifications;
        int _pendingTagNotificationCount;
    };
}
#endif
