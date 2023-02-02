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

#ifndef PMP_RANDOMTRACKSSOURCE_H
#define PMP_RANDOMTRACKSSOURCE_H

#include "common/filehash.h"

#include <QHash>
#include <QObject>
#include <QSet>
#include <QVector>

#include <random>

namespace PMP::Server
{
    class Resolver;

    class RandomTracksSource : public QObject
    {
        Q_OBJECT
    public:
        RandomTracksSource(QObject* parent, Resolver* resolver);

        int totalTrackCount() const { return _hashesStatus.size(); }

        FileHash takeTrack();

        void putBackUsedTrack(const FileHash& hash);
        void putBackUnusedTrack(const FileHash& hash);
        void putBackAllTracksTakenAsUnused();

        void resetUpcomingTrackNotifications();

    Q_SIGNALS:
        void upcomingTrackNotification(FileHash hash);

    private Q_SLOTS:
        void hashBecameAvailable(PMP::FileHash hash);
        void checkNotifications();

    private:
        static const int UPCOMING_NOTIFY_BATCH_COUNT;
        static const int UPCOMING_NOTIFY_TARGET_COUNT;

        enum TrackStatus { Unknown = 0, Unused, Taken, Used };

        TrackStatus getTrackStatus(const FileHash& hash);
        void addNewHashToUnusedList(const FileHash& hash);
        void markUsedTracksAsUnusedAgain();
        void scheduleNotificationsCheck();

        Resolver* _resolver;
        std::mt19937 _randomEngine;
        QVector<FileHash> _unusedHashes;
        QHash<FileHash, TrackStatus> _hashesStatus;
        QSet<FileHash> _hashesTaken;
        int _notifiedCount;
        bool _pendingNotificationsCheck;
    };
}
#endif
