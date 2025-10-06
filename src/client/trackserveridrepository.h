/*
    Copyright (C) 2025, Kevin André <hyperquantum@gmail.com>

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

#ifndef PMP_CLIENT_TRACKSERVERIDREPOSITORY_H
#define PMP_CLIENT_TRACKSERVERIDREPOSITORY_H

#include "common/filehash.h"
#include "common/nullable.h"

#include <QHash>
#include <QReadWriteLock>

namespace PMP::Client
{
    class TrackServerIdRepository
    {
    public:
        TrackServerIdRepository();

        void registerHashWithId(FileHash const& hash, quint64 trackServerId);

        Nullable<quint64> getServerIdForHash(FileHash const& hash) const;
        Nullable<FileHash> getHashForServerId(quint64 trackServerId) const;

    private:
        mutable QReadWriteLock _lock;
        QHash<FileHash, quint64> _hashToServerId;
        QHash<quint64, FileHash> _serverIdToHash;
    };
}
#endif
