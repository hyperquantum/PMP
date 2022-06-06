/*
    Copyright (C) 2022, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_HASHIDREGISTRAR_H
#define PMP_HASHIDREGISTRAR_H

#include "common/filehash.h"
#include "common/future.h"

#include <QHash>
#include <QMutex>
#include <QPair>
#include <QVector>

namespace PMP
{
    class Database;

    class HashIdRegistrar
    {
    public:
        Future<SuccessType, FailureType> loadAllFromDatabase();
        Future<uint, FailureType> getOrCreateId(FileHash hash);
        Future<QVector<uint>, FailureType> getOrCreateIds(QVector<FileHash> hashes);

        QVector<QPair<uint, FileHash>> getAllLoaded();
        QVector<QPair<uint, FileHash>> getExistingIdsOnly(QVector<FileHash> hashes);
        Nullable<FileHash> getHashForId(uint id);

    private:
        ResultOrError<uint, FailureType> registerHash(Database& db, FileHash hash);

        QMutex _mutex;
        QHash<FileHash, uint> _hashes;
        QHash<uint, FileHash> _ids;
    };
}
#endif
