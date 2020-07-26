/*
    Copyright (C) 2014-2020, Kevin Andre <hyperquantum@gmail.com>

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

#include "playerhistoryentry.h"
#include "userdatafortracksfetcher.h" // for UserDataForHashId

#include <QDateTime>
#include <QHash>
#include <QObject>
#include <QSharedPointer>
#include <QTimer>

namespace PMP {

    class FileHash;
    class Player;
    class QueueEntry;

    class History : public QObject {
        Q_OBJECT
    public:
        struct HashStats {
            QDateTime lastHeard;
            qint16 score;
        };

        History(Player* player);

        /** Get last played time since server startup (non-user-specific) */
        QDateTime lastPlayed(FileHash const& hash) const;

        bool fetchMissingUserStats(uint hashID, quint32 user);
        HashStats const* getUserStats(uint hashID, quint32 user);

    Q_SIGNALS:
        void updatedHashUserStats(uint hashID, quint32 user,
                                  QDateTime previouslyHeard, qint16 score);

    private slots:
        void currentTrackChanged(QueueEntry const* newTrack);
        void newHistoryEntry(QSharedPointer<PlayerHistoryEntry> entry);
        void onUpdatedHashUserStats(uint hashID, quint32 user,
                                    QDateTime previouslyHeard, qint16 score);
        void onFetchingTimerTimeout();
        void onFetchCompleted(quint32 userId, QVector<PMP::UserDataForHashId> results);

    private:
        void scheduleFetch(uint hashID, quint32 user);

        static const int fetchingTimerFreqMs = 100;

        Player* _player;
        QHash<FileHash, QDateTime> _lastPlayHash;
        QueueEntry const* _nowPlaying;
        QHash<quint32, QHash<uint, HashStats>> _userStats;
        QHash<quint32, QHash<uint, bool>> _userStatsFetching;
        QTimer* _fetchingTimer;
    };
}
#endif
