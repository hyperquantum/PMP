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

#include "serverhealthmonitor.h"

namespace PMP {

    ServerHealthMonitor::ServerHealthMonitor(QObject* parent)
     : QObject(parent), _databaseUnavailable(false), _sslLibrariesMissing(false)
    {
        //
    }

    bool ServerHealthMonitor::anyProblem() const {
        return _databaseUnavailable || _sslLibrariesMissing;
    }

    bool ServerHealthMonitor::databaseUnavailable() const {
        return _databaseUnavailable;
    }

    bool ServerHealthMonitor::sslLibrariesMissing() const {
        return _sslLibrariesMissing;
    }

    void ServerHealthMonitor::setDatabaseUnavailable() {
        if (_databaseUnavailable) return;

        _databaseUnavailable = true;
        emit serverHealthChanged(_databaseUnavailable, _sslLibrariesMissing);
    }

    void ServerHealthMonitor::setSslLibrariesMissing() {
        if (_sslLibrariesMissing) return;

        _sslLibrariesMissing = true;
        emit serverHealthChanged(_databaseUnavailable, _sslLibrariesMissing);
    }
}
