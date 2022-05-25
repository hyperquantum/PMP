/*
    Copyright (C) 2016-2022, Kevin Andre <hyperquantum@gmail.com>

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

#include "userdatafortracksfetcher.h"

#include "database.h"

#include <QtDebug>

namespace PMP
{
    UserDataForTracksFetcher::UserDataForTracksFetcher(quint32 userId,
                                                       QVector<uint> hashIds)
     : _userId(userId), _hashIds(hashIds)
    {
        //
    }

    void UserDataForTracksFetcher::run()
    {
        auto db = Database::getDatabaseForCurrentThread();
        if (!db) return; /* problem */

        if (_hashIds.size() == 1)
        {
            qDebug() << "fetching track user data for hash ID" << _hashIds[0]
                     << "and user" << _userId;
        }
        else
        {
            qDebug() << "fetching track user data for" << _hashIds.size()
                     << "ID's; user:" << _userId;
        }

        QVector<UserDataForHashId> results;
        results.reserve(_hashIds.size());

        /* convert QVector<uint> to QList<quint32> */
        QList<quint32> ids;
        ids.reserve(_hashIds.size());
        for (auto id : qAsConst(_hashIds))
        {
            ids << id;
        }

        const auto stats = db->getHashHistoryStats(_userId, ids);
        for (auto& stat : stats)
        {
            UserDataForHashId data;
            data.hashId = stat.hashId;
            data.previouslyHeard = stat.lastHeard;
            data.score = stat.score;

            qDebug() << "fetched: user:" << _userId << " hash ID:" << stat.hashId
                     << " score:" << stat.score << " prevHeard:" << stat.lastHeard;

            results.append(data);
        }

        Q_EMIT finishedWithResult(_userId, results);
    }
}
