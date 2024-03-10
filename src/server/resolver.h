/*
    Copyright (C) 2014-2024, Kevin Andre <hyperquantum@gmail.com>

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
#include "common/nullable.h"
#include "common/tagdata.h"

#include "collectiontrackinfo.h"
#include "fileanalysis.h"
#include "filelocations.h"

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
    class Analyzer;
    class FileFinder;
    class FileLocations;
    class HashIdRegistrar;
    class HashRelations;
    class HistoryStatistics;

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
        Future<QString, FailureType> findPathForHashAsync(uint hashId);

        bool haveFileForHash(const FileHash& hash);
        bool pathStillValid(const FileHash& hash, QString path);
        Nullable<FileHash> getHashForFilePath(QString path);

        Nullable<AudioData> findAudioData(const FileHash& hash);
        Nullable<TagData> findTagData(const FileHash& hash);

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
                                QString album, QString albumArtist,
                                qint32 lengthInMilliseconds);

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
    };
}
#endif
