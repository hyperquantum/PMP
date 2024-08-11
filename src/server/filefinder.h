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

#ifndef PMP_SERVER_FILEFINDER_H
#define PMP_SERVER_FILEFINDER_H

#include "common/filehash.h"
#include "common/future.h"

#include "fileanalysis.h"

#include <QHash>
#include <QMutex>
#include <QObject>
#include <QString>

namespace PMP::Server
{
    class Analyzer;
    class Database;
    class FileLocations;
    class HashIdRegistrar;

    class FileFinder : public QObject
    {
        Q_OBJECT
    public:
        FileFinder(QObject* parent, HashIdRegistrar* hashIdRegistrar,
                   FileLocations* fileLocations, Analyzer* analyzer);

        void setMusicPaths(QStringList paths);

        Future<QString, FailureType> findHashAsync(uint id, FileHash hash);

    private Q_SLOTS:
        void fileAnalysisCompleted(QString path, PMP::Server::FileAnalysis analysis);

    private:
        void markAsCompleted(uint id);

        ResultOrError<QString, FailureType> findHashInternal(uint id, FileHash hash);

        QString findPathForHashByLikelyFilename(Database& db, uint id,
                                                FileHash const& hash);

        QString findPathByQuickScanForNewFiles(Database& db, uint id,
                                               const FileHash& hash);

        QString findPathByQuickScanOfNewFiles(QVector<QString> newFiles,
                                              const FileHash& hash);

        QMutex _mutex;
        HashIdRegistrar* _hashIdRegistrar;
        FileLocations* _fileLocations;
        Analyzer* _analyzer;
        QThreadPool* _threadPool;
        QStringList _musicPaths;
        QHash<uint, Future<QString, FailureType>> _inProgress;
    };
}
#endif
