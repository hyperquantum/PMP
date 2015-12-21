/*
    Copyright (C) 2014-2015, Kevin Andre <hyperquantum@gmail.com>

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

#include <random>

#include <QDateTime>
#include <QFutureWatcher>
#include <QHash>
#include <QMultiHash>
#include <QObject>
#include <QReadWriteLock>
#include <QString>

namespace PMP {

    class FileData;
    class TagData;

    class Resolver : public QObject {
        Q_OBJECT
    public:
        Resolver();

        void setMusicPaths(QList<QString> paths);
        QList<QString> musicPaths();

        bool startFullIndexation();
        bool fullIndexationRunning();

        void registerFile(const FileData& file, const QString& filename, qint64 fileSize,
                          QDateTime fileLastModified);

        bool haveFileFor(const FileHash& hash);
        QString findPath(const FileHash& hash, bool fast);
        bool pathStillValid(const FileHash& hash, QString path);

        const AudioData& findAudioData(const FileHash& hash);
        const TagData* findTagData(const FileHash& hash);

        FileHash getRandom();
        QList<FileHash> getAllHashes();
        QList<CollectionTrackInfo> getHashesTrackInfo(QList<FileHash> hashes);

        uint getID(const FileHash& hash);

    private slots:
        void onFullIndexationFinished();

    Q_SIGNALS:
        void fullIndexationRunStatusChanged(bool running);

        void hashBecameAvailable(PMP::FileHash hash);
        void hashBecameUnavailable(PMP::FileHash hash);
        void hashTagInfoChanged(PMP::FileHash hash, QString title, QString artist);

    private:
        struct VerifiedFile;
        class HashKnowledge;

        HashKnowledge* registerHash(const FileHash& hash);
        HashKnowledge* registerData(const FileData& data);

        void registerFile(HashKnowledge* hash, const QString& filename, qint64 fileSize,
                          QDateTime fileLastModified);

        void doFullIndexation();

        QReadWriteLock _lock;
        std::random_device _randomDevice;
        std::mt19937 _randomEngine;

        QList<QString> _musicPaths;

        QList<FileHash> _hashList;
        QHash<FileHash, HashKnowledge*> _hashKnowledge;
        QHash<uint, HashKnowledge*> _idToHash;
        QHash<QString, VerifiedFile*> _paths;
        //QMultiHash<FileHash, VerifiedFile*> _filesForHash;

        bool _fullIndexationRunning;
        QFutureWatcher<void> _fullIndexationWatcher;

        AudioData _emptyAudioData;
    };
}
#endif
