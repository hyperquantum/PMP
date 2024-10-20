/*
    Copyright (C) 2014-2024, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_HISTORY_H
#define PMP_HISTORY_H

#include "common/filehash.h"
#include "common/future.h"
#include "common/nullable.h"

#include "recenthistoryentry.h"
#include "trackstats.h"

#include <QDateTime>
#include <QHash>
#include <QObject>
#include <QSharedPointer>

QT_FORWARD_DECLARE_CLASS(QThreadPool)

namespace PMP::Server
{
    class HashIdRegistrar;
    class HistoryStatistics;
    class Player;
    class QueueEntry;

    class History : public QObject
    {
        Q_OBJECT
    public:
        History(Player* player, HashIdRegistrar* hashIdRegistrar,
                HistoryStatistics* historyStatistics);

        ~History();

        /** Get last played time since server startup (non-user-specific) */
        QDateTime lastPlayedGloballySinceStartup(FileHash const& hash) const;

        Future<SuccessType, FailureType> scheduleUserStatsFetchingIfMissing(
                                                                        uint hashId,
                                                                        quint32 userId);
        Nullable<TrackStats> getUserStats(uint hashId, quint32 userId);

    Q_SIGNALS:
        void hashStatisticsChanged(quint32 userId, QVector<uint> hashIds);

    private Q_SLOTS:
        void currentTrackChanged(QSharedPointer<QueueEntry const> newTrack);
        void newHistoryEntry(QSharedPointer<RecentHistoryEntry> entry);

    private:
        Player* _player;
        HashIdRegistrar* _hashIdRegistrar;
        HistoryStatistics* _statistics;
        QHash<FileHash, QDateTime> _lastPlayHash;
        QSharedPointer<QueueEntry const> _nowPlaying;
    };
}
#endif
