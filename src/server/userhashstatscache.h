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

#ifndef PMP_USERHASHSTATSCACHE_H
#define PMP_USERHASHSTATSCACHE_H

#include "databaserecords.h"

#include <QHash>
#include <QMutex>
#include <QSet>
#include <QVector>

namespace PMP::Server
{
    class UserHashStatsCache
    {
    public:
        UserHashStatsCache();

        bool hasBeenLoadedForUser(quint32 userId);
        void loadForUser(quint32 userId,
                         QVector<DatabaseRecords::HashHistoryStats> stats);

        QVector<DatabaseRecords::HashHistoryStats> getForUser(quint32 userId,
                                                              QVector<uint> hashIds);

        void add(quint32 userId, DatabaseRecords::HashHistoryStats const& stats);
        void remove(quint32 userId, uint hashId);

    private:
        QHash<quint32, QHash<uint, DatabaseRecords::HashHistoryStats>> _stats;
        QSet<quint32> _usersLoaded;
        QMutex _mutex;
    };
}
#endif
