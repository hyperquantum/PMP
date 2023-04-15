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

#include "localhashidrepository.h"

#include "common/filehash.h"

#include <QMutexLocker>
#include <QtDebug>

namespace PMP::Client
{
    LocalHashId LocalHashIdRepository::getOrRegisterId(const FileHash& hash)
    {
        Q_ASSERT_X(hash.isNull() == false,
                   "LocalHashIdRepository::getOrRegisterId",
                   "hash is null");

        QMutexLocker lock(&_mutex);

        auto it = _hashToId.constFind(hash);
        if (it != _hashToId.constEnd())
            return LocalHashId(it.value());

        _lastId++;
        LocalHashId id(_lastId);

        _hashToId.insert(hash, id.value());
        _idToHash.insert(id.value(), hash);

        if (_lastId % 500 == 0)
        {
            qDebug() << "registered local hash ID" << _lastId;
        }

        return id;
    }

    LocalHashId LocalHashIdRepository::getId(const FileHash& hash) const
    {
        if (hash.isNull())
            return {}; // return zero ID

        QMutexLocker lock(&_mutex);

        return LocalHashId(_hashToId.value(hash, 0));
    }

    FileHash LocalHashIdRepository::getHash(LocalHashId id) const
    {
        if (id.isZero())
            return {}; // return null hash

        QMutexLocker lock(&_mutex);

        auto it = _idToHash.constFind(id.value());

        Q_ASSERT_X(it != _idToHash.constEnd(),
                   "LocalHashIdRepository::getHash",
                   "non-zero ID is not found");

        return it.value();
    }
}
