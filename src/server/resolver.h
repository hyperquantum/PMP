/*
    Copyright (C) 2014-2021, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_RESOLVER_H
#define PMP_RESOLVER_H

#include "common/audiodata.h"
#include "common/collectiontrackinfo.h"
#include "common/filehash.h"

#include <QDateTime>
#include <QFutureWatcher>
#include <QHash>
#include <QMultiHash>
#include <QMutex>
#include <QObject>
#include <QPair>
#include <QString>
#include <QVector>

namespace PMP
{
    class Database;
    class TagData;

    class Resolver : public QObject
    {
        Q_OBJECT
    public:
        Resolver();

        void setMusicPaths(QList<QString> paths);
        QList<QString> musicPaths();

        bool startFullIndexation();
        bool fullIndexationRunning();

        FileHash analyzeAndRegisterFile(const QString& filename);

        bool haveFileForHash(const FileHash& hash);
        QString findPathForHash(const FileHash& hash, bool fast);
        bool pathStillValid(const FileHash& hash, QString path);

        const AudioData& findAudioData(const FileHash& hash);
        const TagData* findTagData(const FileHash& hash);

        QVector<FileHash> getAllHashes();
        QVector<CollectionTrackInfo> getHashesTrackInfo(QVector<FileHash> hashes);

        FileHash getHashByID(uint id);
        uint getID(const FileHash& hash);
        QList<QPair<uint, FileHash>> getIDs(QList<FileHash> hashes);
        QVector<QPair<uint, FileHash>> getIDs(QVector<FileHash> hashes);

    private Q_SLOTS:
        void onFullIndexationFinished();

    Q_SIGNALS:
        void fullIndexationRunStatusChanged(bool running);

        void hashBecameAvailable(PMP::FileHash hash);
        void hashBecameUnavailable(PMP::FileHash hash);
        void hashTagInfoChanged(PMP::FileHash hash, QString title, QString artist,
                                QString album, qint32 lengthInMilliseconds);

    private:
        struct VerifiedFile;
        class HashKnowledge;

        FileHash analyzeAndRegisterFileInternal(const QString& filename,
                                                uint fullIndexationNumber);
        HashKnowledge* registerHash(const FileHash& hash);
        QVector<QString> getPathsThatDontMatchCurrentFullIndexationNumber();
        void checkFileStillExistsAndIsValid(QString path);

        QString findPathForHashByLikelyFilename(Database& db, const FileHash& hash,
                                                uint hashId);
        QString findPathByQuickScanForNewFiles(Database& db, const FileHash& hash,
                                               uint hashId);
        QString findPathByQuickScanOfNewFiles(QVector<QString> newFiles,
                                              const FileHash& hash);

        void doFullIndexation();

        QMutex _lock;

        QList<QString> _musicPaths;

        QList<FileHash> _hashList;
        QHash<FileHash, HashKnowledge*> _hashKnowledge;
        QHash<uint, HashKnowledge*> _idToHash;
        QHash<QString, VerifiedFile*> _paths;

        uint _fullIndexationNumber;
        bool _fullIndexationRunning;
        QFutureWatcher<void> _fullIndexationWatcher;

        AudioData _emptyAudioData;
    };
}
#endif
