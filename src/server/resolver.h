/*
    Copyright (C) 2014-2023, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_SERVER_RESOLVER_H
#define PMP_SERVER_RESOLVER_H

#include "common/audiodata.h"
#include "common/filehash.h"
#include "common/future.h"
#include "common/tagdata.h"

#include "analyzer.h"
#include "collectiontrackinfo.h"

#include <QDateTime>
#include <QHash>
#include <QMutex>
#include <QObject>
#include <QPair>
#include <QString>
#include <QStringList>
#include <QVector>

namespace PMP::Server
{
    class Database;
    class HashIdRegistrar;
    class HashRelations;
    class HistoryStatistics;

    class FileLocations
    {
    public:
        void insert(uint id, QString path);
        void remove(uint id, QString path);

        QList<uint> getIdsByPath(QString path);
        QStringList getPathsById(uint id);

        bool pathHasAtLeastOneId(QString path);

    private:
        QMutex _mutex;
        QHash<uint, QStringList> _idToPaths;
        QHash<QString, QList<uint>> _pathToIds;
    };

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

    class Resolver : public QObject
    {
        Q_OBJECT
    public:
        Resolver(HashIdRegistrar* hashIdRegistrar, HashRelations* hashRelations,
                 HistoryStatistics* historyStatistics);

        void setMusicPaths(QStringList paths);
        QStringList musicPaths();

        bool startFullIndexation();
        bool fullIndexationRunning();

        Future<QString, FailureType> findPathForHashAsync(FileHash hash);

        bool haveFileForHash(const FileHash& hash);
        bool pathStillValid(const FileHash& hash, QString path);
        Nullable<FileHash> getHashForFilePath(QString path);

        const AudioData& findAudioData(const FileHash& hash);
        const TagData* findTagData(const FileHash& hash);

        QVector<FileHash> getAllHashes();
        QVector<CollectionTrackInfo> getHashesTrackInfo(QVector<FileHash> hashes);
        CollectionTrackInfo getHashTrackInfo(uint hashId);

        FileHash getHashByID(uint id);
        uint getID(const FileHash& hash);
        QList<QPair<uint, FileHash>> getIDs(QList<FileHash> hashes);
        QVector<QPair<uint, FileHash>> getIDs(QVector<FileHash> hashes);

    private Q_SLOTS:
        void onFullIndexationFinished();
        void onFileAnalysisFailed(QString path);
        void onFileAnalysisCompleted(QString path, FileAnalysis analysis);
        void onAnalyzerFinished();

    Q_SIGNALS:
        void fullIndexationRunStatusChanged(bool running);

        void hashBecameAvailable(PMP::FileHash hash);
        void hashBecameUnavailable(PMP::FileHash hash);
        void hashTagInfoChanged(PMP::FileHash hash, QString title, QString artist,
                                QString album, qint32 lengthInMilliseconds);

    private:
        enum class FullIndexationStatus
        {
            NotRunning,
            FileSystemTraversal,
            WaitingForFileAnalysisCompletion,
            CheckingForFileRemovals,
        };

        struct VerifiedFile;
        class HashKnowledge;

        HashKnowledge* registerHash(const FileHash& hash);
        void markHashesAsEquivalent(QVector<uint> hashes);
        QVector<QString> getPathsThatDontMatchCurrentFullIndexationNumber();
        void checkFileStillExistsAndIsValid(QString path);

        void doFullIndexationFileSystemTraversal();
        void doFullIndexationCheckForFileRemovals();

        FileLocations _fileLocations;
        Analyzer* _analyzer { nullptr };
        FileFinder* _fileFinder { nullptr };
        HashIdRegistrar* _hashIdRegistrar { nullptr };
        HashRelations* _hashRelations { nullptr };
        HistoryStatistics* _historyStatistics { nullptr };

        QMutex _lock;

        QStringList _musicPaths;

        QList<FileHash> _hashList;
        QHash<FileHash, HashKnowledge*> _hashKnowledge;
        QHash<uint, HashKnowledge*> _idToHash;
        QHash<QString, VerifiedFile*> _paths;

        uint _fullIndexationNumber;
        FullIndexationStatus _fullIndexationStatus;

        AudioData _emptyAudioData;
    };
}
#endif
