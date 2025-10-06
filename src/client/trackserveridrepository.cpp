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

#include "trackserveridrepository.h"

namespace PMP::Client
{
    TrackServerIdRepository::TrackServerIdRepository()
    {
        //
    }

    void TrackServerIdRepository::registerHashWithId(const FileHash& hash,
                                                     quint64 trackServerId)
    {
        Q_ASSERT_X(hash.isNull() == false,
                   "TrackServerIdRepository::registerHashWithId",
                   "hash is null");

        Q_ASSERT_X(trackServerId > 0,
                   "TrackServerIdRepository::registerHashWithId",
                   "track server ID is zero");

        QWriteLocker lock(&_lock);

        auto itByHash = _hashToServerId.constFind(hash);
        auto itById = _serverIdToHash.constFind(trackServerId);

        bool hashFound = itByHash != _hashToServerId.constEnd();
        bool idFound = itById != _serverIdToHash.constEnd();

        Q_ASSERT_X(hashFound == idFound,
                   "TrackServerIdRepository::registerHashWithId",
                   "hash or ID not both present or absent in repository");

        if (hashFound)
        {
            Q_ASSERT_X(itByHash.value() == trackServerId,
                       "TrackServerIdRepository::registerHashWithId",
                       "hash in repository linked to different ID");

            Q_ASSERT_X(itById.value() == hash,
                       "TrackServerIdRepository::registerHashWithId",
                       "ID in repository linked to different hash");

            return; /* nothing to register */
        }

        _hashToServerId.insert(hash, trackServerId);
        _serverIdToHash.insert(trackServerId, hash);
    }

    Nullable<quint64> TrackServerIdRepository::getServerIdForHash(
        const FileHash& hash) const
    {
        QReadLocker lock(&_lock);

        auto it = _hashToServerId.constFind(hash);

        if (it == _hashToServerId.constEnd())
            return null;

        return it.value();
    }

    Nullable<FileHash> TrackServerIdRepository::getHashForServerId(
        quint64 trackServerId) const
    {
        QReadLocker lock(&_lock);

        auto it = _serverIdToHash.constFind(trackServerId);

        if (it == _serverIdToHash.constEnd())
            return null;

        return it.value();
    }
}
