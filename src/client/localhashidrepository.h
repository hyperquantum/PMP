/*
    Copyright (C) 2023-2026, Kevin André <hyperquantum@gmail.com>

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

#ifndef PMP_CLIENT_LOCALHASHIDREPOSITORY_H
#define PMP_CLIENT_LOCALHASHIDREPOSITORY_H

#include "common/filehash.h"

#include "localhashid.h"
#include "trackhashorid.h"
#include "trackserverid.h"

#include <QHash>
#include <QMutex>

namespace PMP::Client
{
    enum class LocalTrackIdMode { ServerId, Hash };

    class LocalHashIdRepository
    {
    public:
        void init(LocalTrackIdMode mode);

        LocalTrackIdMode mode() const;

        LocalHashId registerTrack(TrackServerId trackId, FileHash const& hash);
        LocalHashId registerTrackByServerId(TrackServerId trackId);
        LocalHashId registerTrackByHash(FileHash const& hash);

        LocalHashId getIdByServerId(TrackServerId trackId);
        LocalHashId getIdByHash(FileHash const& hash);

        TrackHashOrId getHashOrServerIdByLocalId(LocalHashId id);
        TrackServerId getServerIdByLocalId(LocalHashId id);
        FileHash getHashByLocalId(LocalHashId id);

        LocalHashId tryGetIdByHash(FileHash const& hash);

    private:
        struct LocalIdData
        {
            TrackServerId serverId;
            FileHash hash;
        };

        struct ServerIdData
        {
            LocalHashId localId;
            //FileHash hash;
        };

        struct HashData
        {
            LocalHashId localId;
            //TrackServerId serverId;
        };

        mutable QMutex _mutex;
        uint _lastId {0};
        QHash<LocalHashId, LocalIdData> _localIds;
        QHash<TrackServerId, ServerIdData> _serverIds;
        QHash<FileHash, HashData> _hashes;
        bool _initialized { false };
        LocalTrackIdMode _mode;
    };
}
#endif
