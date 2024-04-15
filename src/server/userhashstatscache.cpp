/*
    Copyright (C) 2024, Kevin Andre <hyperquantum@gmail.com>

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

#include "userhashstatscache.h"

namespace PMP::Server
{
    UserHashStatsCache::UserHashStatsCache()
    {
        //
    }

    bool UserHashStatsCache::hasBeenLoadedForUser(quint32 userId)
    {
        QMutexLocker lock(&_mutex);

        return _usersLoaded.contains(userId);
    }

    void UserHashStatsCache::loadForUser(quint32 userId,
                                         QVector<DatabaseRecords::HashHistoryStats> stats)
    {
        QMutexLocker lock(&_mutex);

        auto& userData = _stats[userId];

        if (stats.size() > userData.size())
            userData.reserve(stats.size());

        for (auto const& record : qAsConst(stats))
        {
            userData.insert(record.hashId, record);
        }

        _usersLoaded << userId;
    }

    QVector<DatabaseRecords::HashHistoryStats> UserHashStatsCache::getForUser(
                                                                    quint32 userId,
                                                                    QVector<uint> hashIds)
    {
        QMutexLocker lock(&_mutex);

        auto& userData = _stats[userId];

        QVector<DatabaseRecords::HashHistoryStats> result;
        result.reserve(hashIds.size());

        for (auto hashId : qAsConst(hashIds))
        {
            auto it = userData.constFind(hashId);

            if (it != userData.constEnd())
            {
                result.append(it.value());
            }
        }

        return result;
    }

    void UserHashStatsCache::add(quint32 userId,
                                 DatabaseRecords::HashHistoryStats const& stats)
    {
        QMutexLocker lock(&_mutex);

        auto& userData = _stats[userId];

        userData.insert(stats.hashId, stats);
    }
}
