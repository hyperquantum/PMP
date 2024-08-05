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

#include "filefinder.h"

//#include "common/concurrent.h"
#include "common/containerutil.h"
#include "common/fileanalyzer.h"
#include "common/newconcurrent.h"

#include "analyzer.h"
#include "database.h"
#include "filelocations.h"
#include "hashidregistrar.h"

#include <QDirIterator>
#include <QFileInfo>
#include <QSet>
#include <QtDebug>
#include <QVector>

namespace PMP::Server
{
    FileFinder::FileFinder(QObject* parent, HashIdRegistrar* hashIdRegistrar,
                           FileLocations* fileLocations, Analyzer* analyzer)
        : QObject(parent),
        _hashIdRegistrar(hashIdRegistrar),
        _fileLocations(fileLocations),
        _analyzer(analyzer),
        _threadPool(new QThreadPool(this))
    {
        /* single thread only, because it's mostly I/O */
        _threadPool->setMaxThreadCount(1);

        connect(analyzer, &Analyzer::fileAnalysisCompleted,
                this, &FileFinder::fileAnalysisCompleted);
    }

    void FileFinder::setMusicPaths(QStringList paths)
    {
        QMutexLocker lock(&_mutex);

        _musicPaths = paths;
        _musicPaths.detach();
    }

    NewFuture<QString, FailureType> FileFinder::findHashAsync(uint id, FileHash hash)
    {
        QMutexLocker lock(&_mutex);

        qDebug() << "FileFinder: need to find hash" << hash << "with ID" << id;

        auto it = _inProgress.find(id);
        if (it != _inProgress.end())
        {
            qDebug() << "FileFinder: returning existing future for ID" << id;
            return it.value();
        }

        qDebug() << "FileFinder: starting background job to find file for ID" << id;

        auto future =
            NewConcurrent::runOnThreadPool<QString, FailureType>(
                _threadPool,
                [this, id, hash]()
                {
                    auto result = findHashInternal(id, hash);
                    markAsCompleted(id);

                    if (result.succeeded())
                    {
                        qDebug() << "FileFinder: found file" << result.result()
                                 << "for ID" << id;
                    }
                    else
                    {
                        qDebug() << "FileFinder: failed to find file for ID" << id;
                    }
                    return result;
                }
            );

        return future;
    }

    void FileFinder::fileAnalysisCompleted(QString path, FileAnalysis analysis)
    {
        for (auto const& hash : analysis.hashes().allHashes())
        {
            auto future = _hashIdRegistrar->getOrCreateId(hash);

            future.handleOnEventLoop(
                this,
                [this, path](FailureOr<uint> outcome)
                {
                    if (outcome.failed())
                        return;

                    auto id = outcome.result();
                    _fileLocations->insert(id, path);
                }
            );
        }
    }

    void FileFinder::markAsCompleted(uint id)
    {
        QMutexLocker lock(&_mutex);
        _inProgress.remove(id);
    }

    ResultOrError<QString, FailureType> FileFinder::findHashInternal(uint id,
                                                                     FileHash hash)
    {
        auto db = Database::getDatabaseForCurrentThread();
        if (!db)
            return failure;

        auto path = findPathForHashByLikelyFilename(*db, id, hash);
        if (!path.isEmpty())
        {
            qDebug() << "FileFinder: found match by filename heuristic:" << path;
            return path;
        }

        path = findPathByQuickScanForNewFiles(*db, id, hash);
        if (!path.isEmpty())
        {
            qDebug() << "FileFinder: found match by quick scan for new files:" << path;
            return path;
        }

        return failure;
    }

    QString FileFinder::findPathForHashByLikelyFilename(Database& db, uint id,
                                                        FileHash const& hash)
    {
        auto filenamesResult = db.getFilenames(id);
        if (filenamesResult.failed()) /* no known filenames */
            return {};

        auto const filenames = filenamesResult.result();

        const auto musicPaths = _musicPaths;
        for (QString const& musicPath : musicPaths)
        {
            QDirIterator it(musicPath, QDir::Dirs | QDir::Readable | QDir::NoDotAndDotDot,
                            QDirIterator::Subdirectories);

            while (it.hasNext())
            {
                QFileInfo entry(it.next());
                if (!entry.isDir()) continue;

                QDir dir(entry.filePath());

                for (QString const& fileShort : filenames)
                {
                    if (!dir.exists(fileShort)) continue;

                    QString candidatePath = dir.filePath(fileShort);

                    auto maybeHash = _analyzer->analyzeFile(candidatePath);
                    if (maybeHash.failed())
                        continue; /* failed to analyze */

                    auto candidateHashes = maybeHash.result().hashes();
                    if (candidateHashes.contains(hash))
                        return candidatePath;
                }
            }
        }

        qDebug() << "FileFinder: filename based heuristic found no results for ID" << id;
        return {};
    }

    QString FileFinder::findPathByQuickScanForNewFiles(Database& db, uint id,
                                                       const FileHash& hash)
    {
        /* get likely file sizes */
        auto previousFileSizesResult = db.getFileSizes(id);
        QSet<qint64> previousFileSizes;
        if (previousFileSizesResult.succeeded())
            ContainerUtil::addToSet(previousFileSizesResult.result(), previousFileSizes);

        QVector<QString> newFilesToScan;

        const auto musicPaths = _musicPaths;
        for (QString const& musicPath : musicPaths)
        {
            QDirIterator it(musicPath, QDirIterator::Subdirectories); /* no symlinks */

            while (it.hasNext())
            {
                QFileInfo entry(it.next());
                if (!FileAnalyzer::isFileSupported(entry)) continue;

                auto candidatePath = entry.absoluteFilePath();

                if (!previousFileSizes.contains(entry.size()))
                {
                    /* file size does not indicate a match */

                    if (!_fileLocations->pathHasAtLeastOneId(candidatePath))
                        newFilesToScan.append(candidatePath); /* it's a new file */
                    continue;
                }

                qDebug() << "FileFinder: checking out" << candidatePath
                         << "because its file size seems to match";
                auto maybeHash = _analyzer->analyzeFile(candidatePath);
                if (maybeHash.failed())
                    continue; /* failed to analyze */

                auto candidateHashes = maybeHash.result().hashes();
                if (candidateHashes.contains(hash))
                    return candidatePath;
            }
        }

        return findPathByQuickScanOfNewFiles(newFilesToScan, hash);
    }

    QString FileFinder::findPathByQuickScanOfNewFiles(QVector<QString> newFiles,
                                                      const FileHash& hash)
    {
        if (newFiles.isEmpty())
            return {};

        const int maxNewFilesToScan = 3;

        const int scanCount = qMin(maxNewFilesToScan, newFiles.size());

        qDebug() << "FileFinder: encountered" << newFiles.size()
                 << "new files; examining" << scanCount << "of them to see if they match";

        for (int i = 0; i < scanCount; ++i)
        {
            auto candidatePath = newFiles[i];

            qDebug() << "FileFinder: checking out new file:" << candidatePath;

            auto maybeHash = _analyzer->analyzeFile(candidatePath);
            if (maybeHash.failed())
                continue; /* failed to analyze */

            auto candidateHashes = maybeHash.result().hashes();
            if (candidateHashes.contains(hash))
                return candidatePath;
        }

        if (scanCount >= newFiles.size())
            return {};

        qDebug() << "FileFinder: reached maximum number of new files to scan;"
                 << "enqueueing" << (newFiles.size() - scanCount) << "files for analysis";

        /* enqueue the rest of the new files for analysis */
        for (int i = scanCount; i < newFiles.size(); ++i)
        {
            _analyzer->enqueueFile(newFiles[i]);
        }

        return {};
    }
}
