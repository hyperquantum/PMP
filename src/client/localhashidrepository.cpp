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

#include "localhashidrepository.h"

#include "common/filehash.h"

#include <QMutexLocker>
#include <QtDebug>

namespace PMP::Client
{
    namespace
    {
        void lastIdWasIncremented(uint lastId)
        {
            if (lastId % 500 == 0)
            {
                qDebug() << "registered local hash ID" << lastId;
            }
        }
    }

    void LocalHashIdRepository::init(LocalTrackIdMode mode)
    {
        Q_ASSERT_X(mode == LocalTrackIdMode::ServerId || mode == LocalTrackIdMode::Hash,
                   "LocalHashIdRepository::init",
                   "mode must be ServerId or Hash");

        QMutexLocker lock(&_mutex);

        Q_ASSERT_X(_initialized == false,
                   "LocalHashIdRepository::init",
                   "cannot initialize twice");

        _mode = mode;
        _initialized = true;
    }

    LocalTrackIdMode LocalHashIdRepository::mode() const
    {
        QMutexLocker lock(&_mutex);
        return _mode;
    }

    LocalHashId LocalHashIdRepository::registerTrack(TrackServerId trackId,
                                                     const FileHash& hash)
    {
        Q_ASSERT_X(trackId.hasValue(),
                   "LocalHashIdRepository::registerTrack",
                   "trackId does not have a value");

        Q_ASSERT_X(hash.isNull() == false,
                   "LocalHashIdRepository::registerTrack",
                   "hash is null");

        QMutexLocker lock(&_mutex);

        Q_ASSERT_X(_initialized,
                   "LocalHashIdRepository::registerTrack",
                   "repository not initialized yet");

        if (_mode == LocalTrackIdMode::ServerId)
        {
            auto serverIdIt = _serverIds.constFind(trackId);
            if (serverIdIt != _serverIds.constEnd())
                return serverIdIt.value().localId;

            _lastId++;
            LocalHashId id(_lastId);

            LocalIdData localIdData = { .serverId = trackId, .hash = {} };
            ServerIdData serverIdData = { .localId = id, /*.hash = {}*/ };

            _localIds.insert(id, localIdData);
            _serverIds.insert(trackId, serverIdData);

            lastIdWasIncremented(_lastId);

            return id;
        }
        else
        {
            auto hashesIt = _hashes.constFind(hash);
            if (hashesIt != _hashes.constEnd())
                return hashesIt.value().localId;

            _lastId++;
            LocalHashId id(_lastId);

            LocalIdData localIdData = { .serverId = {}, .hash = hash };
            HashData hashData = { .localId = id, /*.serverId = {}*/ };

            _localIds.insert(id, localIdData);
            _hashes.insert(hash, hashData);

            lastIdWasIncremented(_lastId);

            return id;
        }
    }

    LocalHashId LocalHashIdRepository::registerTrackByServerId(TrackServerId trackId)
    {
        Q_ASSERT_X(trackId.hasValue(),
                   "LocalHashIdRepository::registerTrackByServerId",
                   "trackId does not have a value");

        QMutexLocker lock(&_mutex);

        Q_ASSERT_X(_initialized,
                   "LocalHashIdRepository::registerTrackByServerId",
                   "repository not initialized yet");

        Q_ASSERT_X(_mode == LocalTrackIdMode::ServerId,
                   "LocalHashIdRepository::registerTrackByServerId",
                   "wrong mode");

        auto serverIdIt = _serverIds.constFind(trackId);
        if (serverIdIt != _serverIds.constEnd())
            return serverIdIt.value().localId;

        _lastId++;
        LocalHashId id(_lastId);

        LocalIdData localIdData = { .serverId = trackId, .hash = {} };
        ServerIdData serverIdData = { .localId = id, /*.hash = {}*/ };

        _localIds.insert(id, localIdData);
        _serverIds.insert(trackId, serverIdData);

        lastIdWasIncremented(_lastId);

        return id;
    }

    LocalHashId LocalHashIdRepository::registerTrackByHash(const FileHash& hash)
    {
        Q_ASSERT_X(hash.isNull() == false,
                   "LocalHashIdRepository::registerTrack",
                   "hash is null");

        QMutexLocker lock(&_mutex);

        Q_ASSERT_X(_initialized,
                   "LocalHashIdRepository::registerTrackByHash",
                   "repository not initialized yet");

        Q_ASSERT_X(_mode == LocalTrackIdMode::Hash,
                   "LocalHashIdRepository::registerTrackByHash",
                   "wrong mode");

        auto hashesIt = _hashes.constFind(hash);
        if (hashesIt != _hashes.constEnd())
            return hashesIt.value().localId;

        _lastId++;
        LocalHashId id(_lastId);

        LocalIdData localIdData = { .serverId = {}, .hash = hash };
        HashData hashData = { .localId = id, /*.serverId = {}*/ };

        _localIds.insert(id, localIdData);
        _hashes.insert(hash, hashData);

        lastIdWasIncremented(_lastId);

        return id;
    }

    LocalHashId LocalHashIdRepository::getIdByServerId(TrackServerId trackId)
    {
        Q_ASSERT_X(trackId.hasValue(),
                   "LocalHashIdRepository::getIdByServerId",
                   "trackId does not have a value");

        QMutexLocker lock(&_mutex);

        Q_ASSERT_X(_initialized,
                   "LocalHashIdRepository::getIdByServerId",
                   "repository not initialized yet");

        Q_ASSERT_X(_mode == LocalTrackIdMode::ServerId,
                   "LocalHashIdRepository::getIdByServerId",
                   "wrong mode");

        auto serverIdIt = _serverIds.constFind(trackId);

        Q_ASSERT_X(serverIdIt != _serverIds.constEnd(),
                   "LocalHashIdRepository::getIdByServerId",
                   "server-side track ID is not registered");

        return serverIdIt.value().localId;
    }

    LocalHashId LocalHashIdRepository::getIdByHash(const FileHash& hash)
    {
        Q_ASSERT_X(hash.isNull() == false,
                   "LocalHashIdRepository::getIdByHash",
                   "hash is null");

        QMutexLocker lock(&_mutex);

        Q_ASSERT_X(_initialized,
                   "LocalHashIdRepository::getIdByHash",
                   "repository not initialized yet");

        Q_ASSERT_X(_mode == LocalTrackIdMode::Hash,
                   "LocalHashIdRepository::getIdByHash",
                   "wrong mode");

        auto hashesIt = _hashes.constFind(hash);

        Q_ASSERT_X(hashesIt != _hashes.constEnd(),
                   "LocalHashIdRepository::getIdByHash",
                   "hash is not registered");

        return hashesIt.value().localId;
    }

    TrackHashOrId LocalHashIdRepository::getHashOrServerIdByLocalId(LocalHashId id)
    {
        Q_ASSERT_X(id.isZero() == false,
                   "LocalHashIdRepository::getHashOrServerIdByLocalId",
                   "ID is zero");

        QMutexLocker lock(&_mutex);

        if (_mode == LocalTrackIdMode::ServerId)
        {
            return _localIds[id].serverId;
        }
        else
        {
            return _localIds[id].hash;
        }
    }

    TrackServerId LocalHashIdRepository::getServerIdByLocalId(LocalHashId id)
    {
        Q_ASSERT_X(id.isZero() == false,
                   "LocalHashIdRepository::getServerIdByLocalId",
                   "ID is zero");

        QMutexLocker lock(&_mutex);

        Q_ASSERT_X(_initialized,
                   "LocalHashIdRepository::getServerIdByLocalId",
                   "repository not initialized yet");

        Q_ASSERT_X(_mode == LocalTrackIdMode::ServerId,
                   "LocalHashIdRepository::getServerIdByLocalId",
                   "wrong mode");

        return _localIds[id].serverId;
    }

    FileHash LocalHashIdRepository::getHashByLocalId(LocalHashId id)
    {
        Q_ASSERT_X(id.isZero() == false,
                   "LocalHashIdRepository::getHashByLocalId",
                   "ID is zero");

        QMutexLocker lock(&_mutex);

        Q_ASSERT_X(_initialized,
                   "LocalHashIdRepository::getHashByLocalId",
                   "repository not initialized yet");

        Q_ASSERT_X(_mode == LocalTrackIdMode::Hash,
                   "LocalHashIdRepository::getHashByLocalId",
                   "wrong mode");

        return _localIds[id].hash;
    }

    LocalHashId LocalHashIdRepository::tryGetIdByHash(const FileHash& hash)
    {
        Q_ASSERT_X(hash.isNull() == false,
                   "LocalHashIdRepository::tryGetIdByHash",
                   "hash is null");

        QMutexLocker lock(&_mutex);

        Q_ASSERT_X(_initialized,
                   "LocalHashIdRepository::tryGetIdByHash",
                   "repository not initialized yet");

        Q_ASSERT_X(_mode == LocalTrackIdMode::Hash,
                   "LocalHashIdRepository::tryGetIdByHash",
                   "wrong mode");

        auto hashesIt = _hashes.constFind(hash);

        if (hashesIt != _hashes.constEnd())
            return hashesIt.value().localId;

        return {};
    }
}
