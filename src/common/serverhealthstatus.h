/*
    Copyright (C) 2018-2021, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_SERVERHEALTHSTATUS_H
#define PMP_SERVERHEALTHSTATUS_H

#include <QMetaType>

namespace PMP
{
    class ServerHealthStatus
    {
    public:
        ServerHealthStatus()
         : _databaseUnavailable(false)
        {
            //
        }

        ServerHealthStatus(bool databaseUnavailable)
         : _databaseUnavailable(databaseUnavailable)
        {
            //
        }

        bool anyProblems() const { return _databaseUnavailable; }
        bool databaseUnavailable() const { return _databaseUnavailable; }

    private:
        bool _databaseUnavailable;
    };

    inline bool operator==(const ServerHealthStatus& me, const ServerHealthStatus& other)
    {
        return me.databaseUnavailable() == other.databaseUnavailable();
    }

    inline bool operator!=(const ServerHealthStatus& me, const ServerHealthStatus& other)
    {
        return !(me == other);
    }
}

Q_DECLARE_METATYPE(PMP::ServerHealthStatus)

#endif
