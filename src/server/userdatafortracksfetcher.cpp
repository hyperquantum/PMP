/*
    Copyright (C) 2016, Kevin Andre <hyperquantum@gmail.com>

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

namespace PMP {

    UserDataForTracksFetcher::UserDataForTracksFetcher(quint32 userId,
                                                       QVector<uint> hashIds)
     : _userId(userId), _hashIds(hashIds)
    {
        //
    }

    void UserDataForTracksFetcher::run() {
        auto db = Database::getDatabaseForCurrentThread();
        if (!db) return; /* problem */

        qDebug() << "FETCHING TRACK USERDATA for" << _hashIds.size() << "tracks";

        QVector<UserDataForHashId> results;
        results.reserve(_hashIds.size());

        /* convert QVector<uint> to QList<quint32> */
        QList<quint32> ids;
        ids.reserve(_hashIds.size());
        Q_FOREACH(auto id, _hashIds) {
            ids << id;
        }

        auto stats = db->getHashHistoryStats(_userId, ids);
        Q_FOREACH(Database::HashHistoryStats const& stat, stats) {
            UserDataForHashId data;
            data.hashId = stat.hashId;
            data.previouslyHeard = stat.lastHeard;
            data.score = stat.score;

            qDebug() << " FETCHED: HashID" << stat.hashId << " Score" << stat.score
                     << " PrevHeard" << stat.lastHeard;

            results.append(data);
        }

        emit finishedWithResult(_userId, results);
    }

    /* =========================== UserDataForHashInit =========================== */

    /* utility object to automatically do the qRegisterMetaType call at program startup */
    class UserDataForTracksInit {
    public:
        UserDataForTracksInit() {
            //qDebug() << "UserDataForTracksInit running";
            qRegisterMetaType<PMP::UserDataForHashId>("PMP::UserDataForHashId");
            qRegisterMetaType<QList<PMP::UserDataForHashId>>(
                                                         "QList<PMP::UserDataForHashId>");
            qRegisterMetaType<QVector<PMP::UserDataForHashId>>(
                                                       "QVector<PMP::UserDataForHashId>");
        }
    };

    static UserDataForTracksInit userDataForTracksInit;

}
