/*
    Copyright (C) 2022-2023, Kevin Andre <hyperquantum@gmail.com>

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

#include "hashidregistrar.h"

#include "common/concurrent.h"
#include "common/containerutil.h"

#include "database.h"

namespace PMP::Server
{
    Future<SuccessType, FailureType> HashIdRegistrar::loadAllFromDatabase()
    {
        auto work =
            [this]() -> ResultOrError<SuccessType, FailureType>
            {
                auto db = Database::getDatabaseForCurrentThread();
                if (!db) return failure; /* database not available */

                auto hashesResult = db->getHashes();
                if (hashesResult.failed())
                    return failure;

                const auto hashes = hashesResult.result();

                QMutexLocker lock(&_mutex);
                for (auto& pair : hashes)
                {
                    _hashes.insert(pair.second, pair.first);
                    _ids.insert(pair.first, pair.second);
                }

                qDebug() << "HashIdRegistrar: loaded" << hashes.count()
                         << "hashes from the database";
                return success;
            };

        return Concurrent::run<SuccessType, FailureType>(work);
    }

    Future<uint, FailureType> HashIdRegistrar::getOrCreateId(FileHash hash)
    {
        {
            QMutexLocker lock(&_mutex);
            auto id = _hashes.value(hash, 0);
            if (id > 0)
                return Future<uint, FailureType>::fromResult(id);
        }

        auto work =
            [this, hash]() -> ResultOrError<uint, FailureType>
            {
                auto db = Database::getDatabaseForCurrentThread();
                if (!db) return failure; /* database not available */

                return registerHash(*db, hash);
            };

        return Concurrent::run<uint, FailureType>(work);
    }

    Future<QVector<uint>, FailureType> HashIdRegistrar::getOrCreateIds(
                                                                 QVector<FileHash> hashes)
    {
        QVector<uint> ids;
        ids.reserve(hashes.size());

        {
            QMutexLocker lock(&_mutex);

            bool incomplete = false;
            for (int i = 0; i < hashes.size(); ++i)
            {
                auto id = _hashes.value(hashes[i], 0);
                ids.append(id);

                if (id <= 0)
                    incomplete = true;
            }

            if (!incomplete)
                return Future<QVector<uint>, FailureType>::fromResult(ids);
        }

        auto work =
            [this, hashes, ids]() -> ResultOrError<QVector<uint>, FailureType>
            {
                auto db = Database::getDatabaseForCurrentThread();
                if (!db) return failure; /* database not available */

                auto result = ids;
                for (int i = 0; i < hashes.size(); ++i)
                {
                    if (result[i] > 0)
                        continue;

                    auto& hash = hashes[i];
                    auto registrationResult = registerHash(*db, hash);
                    if (registrationResult.failed())
                        return failure;

                    result[i] = registrationResult.result();
                }

                return result;
            };

        return Concurrent::run<QVector<uint>, FailureType>(work);
    }

    QVector<QPair<uint, FileHash>> HashIdRegistrar::getAllLoaded()
    {
        QMutexLocker lock(&_mutex);

        QVector<QPair<uint, FileHash>> result;
        result.reserve(_ids.size());

        for (auto it = _ids.begin(); it != _ids.end(); ++it)
        {
            result << qMakePair(it.key(), it.value());
        }

        return result;
    }

    QVector<uint> HashIdRegistrar::getAllIdsLoaded()
    {
        QMutexLocker lock(&_mutex);

        return ContainerUtil::toVector(_ids.keys());
    }

    QVector<QPair<uint, FileHash>> HashIdRegistrar::getExistingIdsOnly(
                                                                 QVector<FileHash> hashes)
    {
        QMutexLocker lock(&_mutex);

        QVector<QPair<uint, FileHash>> result;
        result.reserve(hashes.size());

        for (auto const& hash : hashes)
        {
            auto it = _hashes.constFind(hash);

            if (it == _hashes.constEnd())
                continue;

            QPair<uint, FileHash> pair(it.value(), hash);
            result.append(pair);
        }

        return result;
    }

    Nullable<uint> HashIdRegistrar::getIdForHash(FileHash hash)
    {
        QMutexLocker lock(&_mutex);

        auto it = _hashes.constFind(hash);

        if (it == _hashes.constEnd())
            return null;

        return it.value();
    }

    bool HashIdRegistrar::isRegistered(FileHash hash)
    {
        return getIdForHash(hash).hasValue();
    }

    Nullable<FileHash> HashIdRegistrar::getHashForId(uint id)
    {
        QMutexLocker lock(&_mutex);

        auto it = _ids.constFind(id);
        if (it != _ids.constEnd())
            return it.value();

        return null;
    }

    ResultOrError<uint, FailureType> HashIdRegistrar::registerHash(Database& db,
                                                                   FileHash hash)
    {
        db.registerHash(hash);
        auto idOrError = db.getHashId(hash);
        if (idOrError.failed() || idOrError.result() <= 0)
        {
            qWarning() << "HashIdRegistrar: failed to get/register hash" << hash;
            return failure;
        }

        auto id = idOrError.result();

        QMutexLocker lock(&_mutex);
        _hashes.insert(hash, id);
        _ids.insert(id, hash);
        qDebug() << "HashIdRegistrar: got ID" << id << "for hash" << hash;
        return id;
    }
}
