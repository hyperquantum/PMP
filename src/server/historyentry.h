/*
    Copyright (C) 2020-2023, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_SERVER_HISTORYENTRY_H
#define PMP_SERVER_HISTORYENTRY_H

#include "common/filehash.h"

#include <QDateTime>
#include <QtGlobal>
#include <QVector>

namespace PMP::Server
{
    class HistoryEntry
    {
    public:
        HistoryEntry(FileHash hash, uint user, QDateTime started, QDateTime ended,
                     int permillage, bool validForScoring)
         : _hash(hash), _user(user), _started(started), _ended(ended),
           _permillage(permillage), _validForScoring(validForScoring)
        {
            //
        }

        FileHash hash() const { return _hash; }
        uint userId() const { return _user; }
        QDateTime started() const { return _started; }
        QDateTime ended() const { return _ended; }
        bool validForScoring() const { return _validForScoring; }
        int permillage() const { return _permillage; }

    private:
        FileHash _hash;
        uint _user;
        QDateTime _started;
        QDateTime _ended;
        int _permillage;
        bool _validForScoring;
    };

    class HistoryFragment
    {
    public:
        HistoryFragment(QVector<HistoryEntry> entries, uint lowestEntryId,
                        uint highestEntryId)
         : _entries(entries),
           _lowestEntryId(lowestEntryId),
           _highestEntryId(highestEntryId)
        {
            //
        }

        const QVector<HistoryEntry>& entries() const { return _entries; }
        uint lowestEntryId() const { return _lowestEntryId; }
        uint highestEntryId() const { return _highestEntryId; }

    private:
        QVector<HistoryEntry> _entries;
        uint _lowestEntryId;
        uint _highestEntryId;
    };
}
#endif
