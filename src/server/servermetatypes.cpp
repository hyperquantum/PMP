/*
    Copyright (C) 2018-2023, Kevin Andre <hyperquantum@gmail.com>

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

#include "analyzer.h"
#include "collectiontrackinfo.h"
#include "scrobblingbackend.h"
#include "scrobblingtrack.h"

#include <QMetaType>
#include <QVector>

namespace PMP::Server
{
    /** Utility object to automatically do the qRegisterMetaType calls at program
     *  startup */
    class ServerMetatypesInit
    {
    private:
        ServerMetatypesInit()
        {
            qRegisterMetaType<QVector<uint>>();

            qRegisterMetaType<PMP::Server::CollectionTrackInfo>();
            qRegisterMetaType<PMP::Server::FileAnalysis>();
            qRegisterMetaType<PMP::Server::FileHashes>();
            qRegisterMetaType<PMP::Server::FileInfo>();
            qRegisterMetaType<PMP::Server::ScrobbleResult>();
            qRegisterMetaType<PMP::Server::ScrobblingBackendState>();
            qRegisterMetaType<PMP::Server::ScrobblingTrack>();
        }

        static ServerMetatypesInit GlobalVariable;
    };

    ServerMetatypesInit ServerMetatypesInit::GlobalVariable;
}
