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

#include "addtohistorytask.h"

#include "database.h"
#include "resolver.h"

namespace PMP
{
    AddToHistoryTask::AddToHistoryTask(Resolver* resolver,
                                       QSharedPointer<PlayerHistoryEntry> entry)
     : _resolver(resolver), _entry(entry)
    {
        //
    }

    void AddToHistoryTask::run()
    {
        auto db = Database::getDatabaseForCurrentThread();
        if (!db) return; /* nothing we can do */

        auto& entry = *_entry;
        uint hashId = _resolver->getID(entry.hash());
        uint userId = entry.user();

        bool validForScoring = !(entry.hadError() || entry.hadSeek());

        /* add to history */
        db->addToHistory(
            hashId, userId, entry.started(), entry.ended(), entry.permillage(),
            validForScoring
        );

        /* recalculate user stats */

        QList<quint32> hashes;
        hashes << hashId;
        auto lastHeardResult = db->getHashHistoryStats(userId, hashes);
        QDateTime lastHeard;
        qint16 score = -1;
        if (lastHeardResult.size() == 1 && lastHeardResult[0].hashId == hashId)
        {
            lastHeard = lastHeardResult[0].lastHeard;
            score = lastHeardResult[0].score;
        }

        Q_EMIT updatedHashUserStats(hashId, userId, lastHeard, score);
    }

}
