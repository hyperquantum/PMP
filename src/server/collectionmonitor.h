/*
    Copyright (C) 2015-2016, Kevin Andre <hyperquantum@gmail.com>

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

#include "common/filehash.h"

#include <QHash>
#include <QMetaType>
#include <QObject>
#include <QPair>

namespace PMP {

    class CollectionNotificationInfo {
    public:
        FileHash hash;
        bool isAvailable;
        QString title, artist;
    };

    /*! class that monitors the collection on behalf of connected remotes */
    class CollectionMonitor : public QObject {
        Q_OBJECT
    public:
        struct HashInfo {
            bool isAvailable;
            QString title, artist;

            HashInfo()
             : isAvailable(false)
            {
                //
            }
        };

        CollectionMonitor(QObject* parent = 0);

    public slots:
        void hashBecameAvailable(PMP::FileHash hash);
        void hashBecameUnavailable(PMP::FileHash hash);
        void hashTagInfoChanged(PMP::FileHash hash, QString title, QString artist);

    Q_SIGNALS:
        void hashAvailabilityChanged(QList<QPair<PMP::FileHash, bool> > changes);
        void hashInfoChanged(QList<PMP::CollectionNotificationInfo> changes);

    private slots:
        void emitNotifications();

    private:
        void checkNeedToSendNotifications();
        void emitFullNotifications(QList<FileHash> hashes);
        void emitAvailabilityNotifications(QList<FileHash> hashes);

        struct Changed {
            bool availability;
            bool tags;
        };

        QHash<FileHash, HashInfo> _collection;
        QHash<FileHash, Changed> _pendingNotifications;
        uint _pendingTagNotificationCount;
    };
}

Q_DECLARE_METATYPE(PMP::CollectionNotificationInfo)

#endif