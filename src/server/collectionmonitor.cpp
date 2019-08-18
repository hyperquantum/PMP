/*
    Copyright (C) 2015-2019, Kevin Andre <hyperquantum@gmail.com>

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

#include "collectionmonitor.h"

#include <QtDebug>
#include <QTimer>

namespace PMP {

    CollectionMonitor::CollectionMonitor(QObject* parent)
     : QObject(parent),
       _pendingTagNotificationCount(0)
    {
        //
    }

    void CollectionMonitor::hashBecameAvailable(FileHash hash) {
        HashInfo& info = _collection[hash];
        if (info.isAvailable) return; /* no change */

        info.isAvailable = true;
        _pendingNotifications[hash].availability = true; /* availability changed */
        checkNeedToSendNotifications();
    }

    void CollectionMonitor::hashBecameUnavailable(FileHash hash) {
        HashInfo& info = _collection[hash];
        if (!info.isAvailable) return; /* no change */

        info.isAvailable = false;
        _pendingNotifications[hash].availability = true; /* availability changed */
        checkNeedToSendNotifications();
    }

    void CollectionMonitor::hashTagInfoChanged(FileHash hash,
                                               QString title, QString artist,
                                               QString album, qint32 lengthInMilliseconds)
    {
        HashInfo& info = _collection[hash];
        bool changed = title != info.title || artist != info.artist;
        if (!changed) return;

        info.title = title;
        info.artist = artist;
        info.album = album;
        info.lengthInMilliseconds = lengthInMilliseconds;

        Changed& notification = _pendingNotifications[hash];
        if (!notification.tags) {
            notification.tags = true; /* tag info changed */
            _pendingTagNotificationCount++;
            checkNeedToSendNotifications();
        }
    }

    void CollectionMonitor::checkNeedToSendNotifications() {
        bool first = _pendingNotifications.size() == 1;

        if (first) {
            QTimer::singleShot(1500, this, &CollectionMonitor::emitNotifications);
        }
        else if (_pendingNotifications.size() >= 50) {
            /* many notifications pending, don't wait */
            QTimer::singleShot(0, this, &CollectionMonitor::emitNotifications);
        }
    }

    void CollectionMonitor::emitNotifications() {
        if (_pendingNotifications.isEmpty()) return;

        if (_pendingTagNotificationCount >= (uint)_pendingNotifications.size()) {
            qDebug() << "CollectionMonitor: going to send" << _pendingNotifications.size()
                     << "full notifications";

            emitFullNotifications(_pendingNotifications.keys());
        }
        else if (_pendingTagNotificationCount == 0) {
            qDebug() << "CollectionMonitor: going to send" << _pendingNotifications.size()
                     << "availability notifications";

            emitAvailabilityNotifications(_pendingNotifications.keys());
        }
        else {
            /* we need to split the list */

            QList<FileHash> fullNotifications;
            QList<FileHash> availabilityNotifications;
            fullNotifications.reserve(_pendingTagNotificationCount);
            availabilityNotifications.reserve(
                _pendingNotifications.size() - _pendingTagNotificationCount
            );

            QHashIterator<FileHash, Changed> i(_pendingNotifications);
            while (i.hasNext()) {
                i.next();
                if (i.value().tags) {
                    fullNotifications.append(i.key());
                }
                else {
                    availabilityNotifications.append(i.key());
                }
            }

            qDebug() << "CollectionMonitor: going to send" << fullNotifications.size()
                     << "full notifications and" << availabilityNotifications.size()
                     << "availability notifications";

            emitFullNotifications(fullNotifications);
            emitAvailabilityNotifications(availabilityNotifications);
        }

        _pendingNotifications.clear();
        _pendingTagNotificationCount = 0;
    }

    void CollectionMonitor::emitFullNotifications(QList<FileHash> hashes) {
        QList<CollectionTrackInfo> notifications;
        notifications.reserve(hashes.size());

        Q_FOREACH(FileHash h, hashes) {
            auto it = _collection.find(h);
            if (it == _collection.end()) continue; /* disappeared?? */

            const HashInfo& value = it.value();
            CollectionTrackInfo info(h, value.isAvailable, value.title, value.artist,
                                     value.album, value.lengthInMilliseconds);
            notifications.append(info);
        }

        emit hashInfoChanged(notifications);
    }

    void CollectionMonitor::emitAvailabilityNotifications(QList<FileHash> hashes) {
        QList<QPair<FileHash, bool> > notifications;
        notifications.reserve(hashes.size());

        Q_FOREACH(FileHash h, hashes) {
            auto it = _collection.find(h);
            if (it == _collection.end()) continue; /* disappeared?? */

            notifications.append(qMakePair(h, it.value().isAvailable));
        }

        emit hashAvailabilityChanged(notifications);
    }
}
