/*
    Copyright (C) 2023, Kevin Andre <hyperquantum@gmail.com>

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

#include <QHash>
#include <QMutex>

namespace PMP::Client
{
    class LocalHashIdRepository
    {
    public:
        LocalHashId getOrRegisterId(FileHash const& hash);
        LocalHashId getId(FileHash const& hash) const;

        FileHash getHash(LocalHashId id) const;

    private:
        mutable QMutex _mutex;
        uint _lastId {0};
        QHash<FileHash, uint> _hashToId;
        QHash<uint, FileHash> _idToHash;
    };
}
#endif
