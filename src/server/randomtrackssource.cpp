/*
    Copyright (C) 2020, Kevin Andre <hyperquantum@gmail.com>

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

#include "randomtrackssource.h"

#include "resolver.h"

#include "common/util.h"

#include <QTimer>

#include <algorithm>

namespace PMP {

    const int RandomTracksSource::UPCOMING_NOTIFY_BATCH_COUNT = 10;
    const int RandomTracksSource::UPCOMING_NOTIFY_TARGET_COUNT = 250;

    RandomTracksSource::RandomTracksSource(QObject* parent, Resolver* resolver)
     : QObject(parent),
       _resolver(resolver),
       _randomEngine(Util::getRandomSeed()),
       _notifiedCount(0),
       _pendingNotificationsCheck(false)
    {
        connect(
            _resolver, &Resolver::hashBecameAvailable,
            this, &RandomTracksSource::hashBecameAvailable
        );

        _unusedHashes = _resolver->getAllHashes();
        std::shuffle(_unusedHashes.begin(), _unusedHashes.end(), _randomEngine);

        _hashesStatus.reserve(_unusedHashes.size());
        for (auto hash : _unusedHashes) {
            _hashesStatus.insert(hash, TrackStatus::Unused);
        }

        qDebug() << "random tracks source initialized; track count: "
                 << _unusedHashes.size();

        scheduleNotificationsCheck();
    }

    FileHash RandomTracksSource::takeTrack()
    {
        if (_unusedHashes.isEmpty()) {
            /* start over */
            markUsedTracksAsUnusedAgain();

            /* still empty? */
            if (_unusedHashes.isEmpty())
                return FileHash(); /* no result */
        }

        auto hash = _unusedHashes.takeLast();
        _hashesStatus.insert(hash, TrackStatus::Taken);
        _hashesTaken.insert(hash);

        auto takenCount = _hashesTaken.size();
        auto unusedHashCount = _unusedHashes.size();
        auto usedHashCount = _hashesStatus.size() - unusedHashCount - takenCount;

        if (_notifiedCount > 0) {
            _notifiedCount--;
        }

        if (unusedHashCount % 10 == 0) {
            qDebug() << "unused tracks list down to" << unusedHashCount
                     << "elements; taken count:" << takenCount
                     << "; used count:" << usedHashCount
                     << "; notified count:" << _notifiedCount;
        }

        scheduleNotificationsCheck();

        return hash;
    }

    void RandomTracksSource::putBackUsedTrack(const FileHash& hash)
    {
        auto status = getTrackStatus(hash);
        if (status != TrackStatus::Taken) {
            qWarning() << "track status for hash" << hash << "expected to be Taken but is"
                       << int(status);
            return;
        }

        _hashesStatus.insert(hash, TrackStatus::Used);
        _hashesTaken.remove(hash);
    }

    void RandomTracksSource::putBackUnusedTrack(const FileHash& hash)
    {
        auto status = getTrackStatus(hash);
        if (status != TrackStatus::Taken) {
            qWarning() << "track status for hash" << hash << "expected to be Taken but is"
                       << int(status);
            return;
        }

        _notifiedCount++;
        _unusedHashes.append(hash);
        _hashesStatus.insert(hash, TrackStatus::Unused);
        _hashesTaken.remove(hash);
    }

    void RandomTracksSource::putBackAllTracksTakenAsUnused()
    {
        for (auto hash : _hashesTaken) {
            _notifiedCount++;
            _unusedHashes.append(hash);
            _hashesStatus.insert(hash, TrackStatus::Unused);
        }

        _hashesTaken.clear();
    }

    void RandomTracksSource::resetUpcomingTrackNotifications()
    {
        qDebug() << "resetting notified count";
        _notifiedCount = 0;
        scheduleNotificationsCheck();
    }

    void RandomTracksSource::hashBecameAvailable(FileHash hash)
    {
        auto status = getTrackStatus(hash);
        switch (status) {
            case TrackStatus::Unknown:
                addNewHashToUnusedList(hash);
                return;
            case TrackStatus::Unused:
            case TrackStatus::Taken:
            case TrackStatus::Used:
                return; /* no action needed */
        }
    }

    void RandomTracksSource::checkNotifications()
    {
        _pendingNotificationsCheck = false;

        if (_notifiedCount >= UPCOMING_NOTIFY_TARGET_COUNT)
            return;

        for (int i = 0; i < UPCOMING_NOTIFY_BATCH_COUNT; ++i) {
            auto index = _unusedHashes.size() - 1 - _notifiedCount;
            if (index < 0) break;

            _notifiedCount++;
            Q_EMIT upcomingTrackNotification(_unusedHashes[index]);
        }

        qDebug() << "notified count has reached" << _notifiedCount;

        scheduleNotificationsCheck();
    }

    RandomTracksSource::TrackStatus RandomTracksSource::getTrackStatus(
                                                                     const FileHash& hash)
    {
        return _hashesStatus.value(hash, TrackStatus::Unknown);
    }

    void RandomTracksSource::addNewHashToUnusedList(const FileHash& hash)
    {
        /* put it at a random index in the unused list */

        /* pick a random index to insert it */
        int endIndex = _unusedHashes.size();
        std::uniform_int_distribution<int> range(0, endIndex);
        int randomIndex = range(_randomEngine);

        _hashesStatus.insert(hash, TrackStatus::Unused);

        /* Inserting into the middle of a list can be an expensive operation. Avoid it by
         * appending the new hash and then swapping it with the element at the target
         * index. We can do this because the list is in random order anyway. */
        _unusedHashes.append(hash);
        if (randomIndex < endIndex) {
            std::swap(_unusedHashes[randomIndex], _unusedHashes[endIndex]);

            _notifiedCount = qMin(_notifiedCount, endIndex - randomIndex);
            Q_EMIT upcomingTrackNotification(_unusedHashes[endIndex]);
        }
        else {
            _notifiedCount++;
            Q_EMIT upcomingTrackNotification(_unusedHashes[endIndex]);
        }

        scheduleNotificationsCheck();
    }

    void RandomTracksSource::markUsedTracksAsUnusedAgain()
    {
        qDebug() << "rebuilding list of unused tracks";

        for (auto it = _hashesStatus.begin(); it != _hashesStatus.end(); ++it) {
            if (it.value() != TrackStatus::Used)
                continue;

            _unusedHashes.append(it.key());
            it.value() = TrackStatus::Unused;
        }

        std::shuffle(_unusedHashes.begin(), _unusedHashes.end(), _randomEngine);

        resetUpcomingTrackNotifications();
    }

    void RandomTracksSource::scheduleNotificationsCheck()
    {
        if (_pendingNotificationsCheck)
            return;

        _pendingNotificationsCheck = true;
        QTimer::singleShot(0, this, &RandomTracksSource::checkNotifications);
    }

}
