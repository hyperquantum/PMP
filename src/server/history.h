/*
    Copyright (C) 2014-2022, Kevin Andre <hyperquantum@gmail.com>

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
#include "common/nullable.h"
#include "common/resultorerror.h"

#include "historystatistics.h"
#include "playerhistoryentry.h"

#include <QDateTime>
#include <QHash>
#include <QMutex>
#include <QObject>
#include <QSet>
#include <QSharedPointer>

QT_FORWARD_DECLARE_CLASS(QThreadPool)

namespace PMP
{
    class FileHash;
    class HashIdRegistrar;
    class HashRelations;
    class Player;
    class QueueEntry;

    class History : public QObject
    {
        Q_OBJECT
    public:
        History(Player* player, HashIdRegistrar* hashIdRegistrar,
                HashRelations* hashRelations);

        ~History();

        /** Get last played time since server startup (non-user-specific) */
        QDateTime lastPlayedGloballySinceStartup(FileHash const& hash) const;

        void scheduleUserStatsFetchingIfMissing(uint hashId, quint32 userId);
        Nullable<TrackStats> getUserStats(uint hashId, quint32 userId);

    Q_SIGNALS:
        void hashStatisticsChanged(quint32 userId, QVector<uint> hashIds);

    private Q_SLOTS:
        void currentTrackChanged(QSharedPointer<QueueEntry const> newTrack);
        void newHistoryEntry(QSharedPointer<PlayerHistoryEntry> entry);

    private:
        void scheduleFetch(uint hashId, quint32 userId);

        QThreadPool* _threadPool;
        Player* _player;
        HashIdRegistrar* _hashIdRegistrar;
        HashRelations* _hashRelations;
        HistoryStatistics* _statistics;
        QHash<FileHash, QDateTime> _lastPlayHash;
        QSharedPointer<QueueEntry const> _nowPlaying;
        QHash<quint32, QSet<uint>> _userDataBeingFetched;
    };
}
#endif
