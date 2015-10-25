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
#include "common/filehash.h"

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

        bool haveAnyPathInfo(const FileHash& hash);
        QString findPath(const FileHash& hash, bool fast);
        bool pathStillValid(const FileHash& hash, QString path);

        const AudioData& findAudioData(const FileHash& hash);
        const TagData* findTagData(const FileHash& hash);

        FileHash getRandom();

        uint getID(const FileHash& hash);

    private slots:
        void onFullIndexationFinished();

    Q_SIGNALS:
        void fullIndexationRunStatusChanged(bool running);

    private:
        struct VerifiedFile {
            QString _path;
            qint64 _size;
            QDateTime _lastModifiedUtc;
            FileHash _hash;

            VerifiedFile(QString path, qint64 size, QDateTime lastModified, FileHash hash)
             : _path(path), _size(size), _lastModifiedUtc(lastModified.toUTC()),
               _hash(hash)
            {
                //
            }

            bool equals(FileHash const& hash, qint64 size, QDateTime modified) {
                return _hash == hash && _size == size
                    && _lastModifiedUtc == modified.toUTC();
            }

            bool equals(qint64 size, QDateTime modified) {
                return _size == size && _lastModifiedUtc == modified.toUTC();
            }

            bool stillValid();
        };

        class HashKnowledge {
            FileHash _hash;
            uint _hashId;
            AudioData _audio;
            QList<const TagData*> _tags;

        public:
            HashKnowledge(FileHash hash, uint hashId)
             : _hash(hash), _hashId(hashId)
            {
                //
            }

            const FileHash& hash() const { return _hash; }

            uint id() const { return _hashId; }
            void setId(uint id) { _hashId = id; }

            const AudioData& audio() const { return _audio; }
            AudioData& audio() { return _audio; }
            void addAudioInfo(AudioData const& audio);

            void addTags(const TagData* t);

            //operator const FileHash& () const { return _hash; }

            TagData const* findBestTag();

        };

        HashKnowledge* registerHash(const FileHash& hash);
        HashKnowledge* registerData(const FileData& data);

        void registerFile(HashKnowledge* hash, const QString& filename, qint64 fileSize,
                          QDateTime fileLastModified);

        void doFullIndexation();

        QReadWriteLock _lock;

        QList<QString> _musicPaths;

        QList<FileHash> _hashList;
        QHash<FileHash, HashKnowledge*> _hashKnowledge;
        QHash<uint, HashKnowledge*> _idToHash;

        QMultiHash<FileHash, VerifiedFile*> _filesForHash;
        QHash<QString, VerifiedFile*> _paths;

        bool _fullIndexationRunning;
        QFutureWatcher<void> _fullIndexationWatcher;

        AudioData _emptyAudioData;
    };
}
#endif
