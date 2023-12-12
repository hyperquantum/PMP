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

#ifndef PMP_RECENTHISTORYENTRY_H
#define PMP_RECENTHISTORYENTRY_H

#include "common/filehash.h"

#include <QDateTime>
#include <QtGlobal>

namespace PMP::Server
{
    class RecentHistoryEntry
    {
    public:
        RecentHistoryEntry(uint queueId, FileHash hash, uint user,
                           QDateTime started, QDateTime ended,
                           bool hadError, bool hadSeek, int permillage)
         : _queueId(queueId), _hash(hash), _user(user), _started(started), _ended(ended),
           _permillage(permillage), _error(hadError), _seek(hadSeek)
        {
            //
        }

        uint queueID() const { return _queueId; }
        FileHash hash() const { return _hash; }
        uint user() const { return _user; }
        QDateTime started() const { return _started; }
        QDateTime ended() const { return _ended; }
        bool hadError() const { return _error; }
        bool hadSeek() const { return _seek; }
        int permillage() const { return _permillage; }

    private:
        uint _queueId;
        FileHash _hash;
        uint _user;
        QDateTime _started;
        QDateTime _ended;
        int _permillage;
        bool _error;
        bool _seek;
    };
}
#endif
