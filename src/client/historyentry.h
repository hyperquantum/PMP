/*
    Copyright (C) 2020-2026, Kevin André <hyperquantum@gmail.com>

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

#ifndef PMP_CLIENT_HISTORYENTRY_H
#define PMP_CLIENT_HISTORYENTRY_H

#include "localtrackid.h"

#include <QDateTime>
#include <QtGlobal>

namespace PMP::Client
{
    class HistoryEntry
    {
    public:
        HistoryEntry(LocalTrackId hashId, uint user, QDateTime started, QDateTime ended,
                     int permillage, bool validForScoring)
         : _hashId(hashId), _user(user), _started(started), _ended(ended),
           _permillage(permillage), _validForScoring(validForScoring)
        {
            //
        }

        LocalTrackId hashId() const { return _hashId; }
        uint userId() const { return _user; }
        QDateTime started() const { return _started; }
        QDateTime ended() const { return _ended; }
        bool validForScoring() const { return _validForScoring; }
        int permillage() const { return _permillage; }

    private:
        LocalTrackId _hashId;
        uint _user;
        QDateTime _started;
        QDateTime _ended;
        int _permillage;
        bool _validForScoring;
    };

    class HistoryFragment
    {
    public:
        HistoryFragment(QVector<HistoryEntry> entries, uint nextStartId)
         : _entries(entries),
           _nextStartId(nextStartId)
        {
            //
        }

        const QVector<HistoryEntry>& entries() const { return _entries; }
        uint nextStartId() const { return _nextStartId; }

    private:
        QVector<HistoryEntry> _entries;
        uint _nextStartId;
    };
}
#endif
