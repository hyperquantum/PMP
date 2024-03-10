/*
    Copyright (C) 2022-2024, Kevin Andre <hyperquantum@gmail.com>

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

#include "filelocations.h"

#include <QtDebug>

namespace PMP::Server
{
    void FileLocations::insert(uint id, QString path)
    {
        if (id <= 0)
        {
            qWarning() << "FileLocations: insert() called with invalid ID" << id
                       << "for path" << path;
            return;
        }

        if (path.isEmpty())
        {
            qWarning() << "FileLocations: insert() called with empty path for ID" << id;
            return;
        }

        QMutexLocker lock(&_mutex);

        auto& paths = _idToPaths[id];
        if (!paths.contains(path))
            paths.append(path);

        auto& ids = _pathToIds[path];
        if (!ids.contains(id))
            ids.append(id);
    }

    void FileLocations::remove(uint id, QString path)
    {
        qDebug() << "FileLocations: remove() called for ID" << id << "and path" << path;

        if (id <= 0)
        {
            qWarning() << "FileLocations: remove() called with invalid ID" << id
                       << "for path" << path;
            return;
        }

        if (path.isEmpty())
        {
            qWarning() << "FileLocations: remove() called with empty path for ID" << id;
            return;
        }

        QMutexLocker lock(&_mutex);

        auto& paths = _idToPaths[id];
        paths.removeOne(path);

        auto& ids = _pathToIds[path];
        ids.removeOne(id);
    }

    QList<uint> FileLocations::getIdsByPath(QString path)
    {
        QMutexLocker lock(&_mutex);

        auto it = _pathToIds.find(path);
        if (it == _pathToIds.end())
            return {};

        auto ids = it.value();
        ids.detach();

        return ids;
    }

    QStringList FileLocations::getPathsById(uint id)
    {
        QMutexLocker lock(&_mutex);

        auto it = _idToPaths.find(id);
        if (it == _idToPaths.end())
            return {};

        auto paths = it.value();
        paths.detach();

        return paths;
    }

    bool FileLocations::pathHasAtLeastOneId(QString path)
    {
        QMutexLocker lock(&_mutex);

        auto it = _pathToIds.find(path);
        if (it == _pathToIds.end())
            return false;

        return !it.value().isEmpty();
    }
}
