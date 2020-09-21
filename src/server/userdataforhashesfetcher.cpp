/*
    Copyright (C) 2016-2020, Kevin Andre <hyperquantum@gmail.com>

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

#include "userdataforhashesfetcher.h"

#include "database.h"
#include "resolver.h"

#include <QtDebug>

namespace PMP {

    UserDataForHashesFetcher::UserDataForHashesFetcher(quint32 userId,
                                                       QVector<FileHash> hashes,
                                                       bool previouslyHeard, bool score,
                                                       Resolver& resolver)
        : _userId(userId), _hashes(hashes), _resolver(resolver),
          _previouslyHeard(previouslyHeard), _score(score)
    {
        //
    }

    void UserDataForHashesFetcher::run() {
        auto db = Database::getDatabaseForCurrentThread();
        if (!db) return; /* problem */

        qDebug() << "FETCHING USERDATA; PrevHeard" << _previouslyHeard
                 << ", Score" << _score;

        QVector<UserDataForHash> results;
        results.reserve(_hashes.size());

        auto idsForHashes = _resolver.getIDs(_hashes);
        QHash<quint32, FileHash> ids;
        ids.reserve(idsForHashes.size());
        Q_FOREACH(auto idAndHash, idsForHashes) {
            ids.insert(idAndHash.first, idAndHash.second);
        }

        if (_score) {
            /* get score and last heard */
            auto stats = db->getHashHistoryStats(_userId, ids.keys());
            Q_FOREACH(Database::HashHistoryStats const& stat, stats) {
                UserDataForHash data;
                data.hash = ids.value(stat.hashId);
                data.previouslyHeard = stat.lastHeard;
                data.score = stat.score;

                qDebug() << "FETCHED: User" << _userId << " HashID" << stat.hashId
                         << " PrevHeard" << stat.lastHeard << " Score" << stat.score;

                results.append(data);
            }
        }
        else {
            /* only last heard */
            auto lastHeardList = db->getLastHeard(_userId, ids.keys());
            Q_FOREACH(auto lastHeard, lastHeardList) {
                UserDataForHash data;
                data.hash = ids.value(lastHeard.first);
                data.previouslyHeard = lastHeard.second;

                results.append(data);
            }
        }

        Q_EMIT finishedWithResult(_userId, results, _previouslyHeard, _score);
    }

}
