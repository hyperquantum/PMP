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

#include "userdataforhashesfetcher.h"

#include "database.h"
#include "resolver.h"

//#include <QtDebug>

namespace PMP {

    UserDataForHashesFetcher::UserDataForHashesFetcher(quint32 userId,
                                                       QList<FileHash> hashes,
                                                       Resolver& resolver)
        : _userId(userId), _hashes(hashes), _resolver(resolver)
    {
        //
    }

    void UserDataForHashesFetcher::run() {
        auto db = Database::getDatabaseForCurrentThread();

        if (!db) return; /* problem */

        QList<UserDataForHash> results;
        results.reserve(_hashes.size());

        auto idsForHashes = _resolver.getIDs(_hashes);
        QHash<quint32, FileHash> ids;
        //QList<quint32> ids;
        ids.reserve(idsForHashes.size());
        Q_FOREACH(auto idAndHash, idsForHashes) {
            //ids.append(idAndHash.first);
            ids.insert(idAndHash.first, idAndHash.second);
        }

        auto lastHeardList = db->getLastHeard(_userId, ids.keys());
        Q_FOREACH(auto lastHeard, lastHeardList) {
            UserDataForHash data;
            data.hash = ids.value(lastHeard.first);
            data.previouslyHeard = lastHeard.second;

            results.append(data);
        }

        emit finishedWithResult(_userId, results);
    }

    /* =========================== UserDataForHashInit =========================== */

    /* utility object to automatically do the qRegisterMetaType call at program startup */
    class UserDataForHashInit {
    public:
        UserDataForHashInit() {
            //qDebug() << "UserDataForHashInit running";
            qRegisterMetaType<PMP::UserDataForHash>("PMP::UserDataForHash");
            qRegisterMetaType<QList<PMP::UserDataForHash> >("QList<PMP::UserDataForHash>");
        }
    };

    static UserDataForHashInit userDataForHashInit;

}
