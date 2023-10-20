/*
    Copyright (C) 2018-2022, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_SERVERHEALTHMONITOR_H
#define PMP_SERVERHEALTHMONITOR_H

#include <QObject>

namespace PMP::Server
{
    /*! class that monitors server health status on behalf of connected remotes */
    class ServerHealthMonitor : public QObject
    {
        Q_OBJECT
    public:
        explicit ServerHealthMonitor(QObject *parent = nullptr);

        bool anyProblem() const;
        bool databaseUnavailable() const;
        bool sslLibrariesMissing() const;

    public Q_SLOTS:
        void setDatabaseUnavailable();
        void setSslLibrariesMissing();

    Q_SIGNALS:
        void serverHealthChanged(bool databaseUnavailable, bool sslLibrariesMissing);

    private:
        bool _databaseUnavailable;
        bool _sslLibrariesMissing;
    };
}
#endif
