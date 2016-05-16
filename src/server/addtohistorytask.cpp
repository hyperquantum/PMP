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

#include "addtohistorytask.h"

#include "database.h"

namespace PMP {

    AddToHistoryTask::AddToHistoryTask(uint hashID, quint32 user, QDateTime started,
                                       QDateTime ended, int permillage,
                                       bool validForScoring)
     : _hashID(hashID), _user(user), _started(started), _ended(ended),
       _permillage(permillage), _validForScoring(validForScoring)
    {
        //
    }

    void AddToHistoryTask::run() {
        auto db = Database::getDatabaseForCurrentThread();
        if (!db) return; /* nothing we can do */

        auto userId = _user;
        auto hashId = _hashID;

        /* add to history */
        db->addToHistory(
            hashId, userId, _started, _ended, _permillage, _validForScoring
        );

        /* recalculate user stats */

        QList<quint32> hashes;
        hashes << hashId;
        auto lastHeardResult = db->getLastHeard(userId, hashes);
        QDateTime lastHeard;
        if (lastHeardResult.size() == 1 && lastHeardResult[0].first == hashId) {
            lastHeard = lastHeardResult[0].second;
        }

        emit updatedHashUserStats(hashId, userId, lastHeard);
    }

}
