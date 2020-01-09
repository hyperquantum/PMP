/*
    Copyright (C) 2018-2020, Kevin Andre <hyperquantum@gmail.com>

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

namespace PMP {

    class ServerHealthStatus {
    public:
        ServerHealthStatus()
         : _databaseUnavailable(false), _sslLibrariesMissing(false),
           _unspecifiedProblems(false)
        {
            //
        }

        ServerHealthStatus(bool databaseUnavailable, bool sslLibrariesMissing,
                           bool unspecifiedProblems)
         : _databaseUnavailable(databaseUnavailable),
           _sslLibrariesMissing(sslLibrariesMissing),
           _unspecifiedProblems(unspecifiedProblems)
        {
            //
        }

        bool anyProblems() const {
            return _databaseUnavailable || _sslLibrariesMissing || _unspecifiedProblems;
        }

        bool databaseUnavailable() const { return _databaseUnavailable; }
        bool sslLibrariesMissing() const { return _sslLibrariesMissing; }
        bool unspecifiedProblems() const { return _unspecifiedProblems; }

    private:
        bool _databaseUnavailable;
        bool _sslLibrariesMissing;
        bool _unspecifiedProblems;
    };

    inline bool operator==(const ServerHealthStatus& me, const ServerHealthStatus& other)
    {
        return me.databaseUnavailable() == other.databaseUnavailable()
                && me.sslLibrariesMissing() == other.sslLibrariesMissing()
                && me.unspecifiedProblems() == other.unspecifiedProblems();
    }

    inline bool operator!=(const ServerHealthStatus& me, const ServerHealthStatus& other)
    {
        return !(me == other);
    }

}

Q_DECLARE_METATYPE(PMP::ServerHealthStatus)

#endif // SERVERHEALTHSTATUS_H
