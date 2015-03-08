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
#include "common/hashid.h"

#include <QDateTime>
#include <QHash>
#include <QMultiHash>
#include <QObject>
#include <QString>

namespace PMP {

    class FileData;
    class TagData;

    class Resolver : public QObject {
        Q_OBJECT
    public:
        Resolver();

        void setMusicPaths(QList<QString> paths);

        void registerData(const HashID& hash, const AudioData& data);
        void registerData(const FileData& data);

        void registerFile(const FileData& file, const QString& filename, qint64 fileSize,
                          QDateTime fileLastModified);
        void registerFile(const HashID& hash, const QString& filename, qint64 fileSize,
                          QDateTime fileLastModified);

        bool haveAnyPathInfo(const HashID& hash);
        QString findPath(const HashID& hash, bool fast);
        bool pathStillValid(const HashID& hash, QString path);

        const AudioData& findAudioData(const HashID& hash);
        const TagData* findTagData(const HashID& hash);

        HashID getRandom();

        uint getID(const HashID& hash) const;

    public slots:
        void analysedFile(QString filename, qint64 fileSize, QDateTime fileLastModified,
                          FileData* data);

    private:
        struct VerifiedFile {
            QString _path;
            qint64 _size;
            QDateTime _lastModifiedUtc;
            HashID _hash;

            VerifiedFile(QString path, qint64 size, QDateTime lastModified, HashID hash)
             : _path(path), _size(size), _lastModifiedUtc(lastModified.toUTC()),
               _hash(hash)
            {
                //
            }

            bool equals(HashID const& hash, qint64 size, QDateTime modified) {
                return _hash == hash && _size == size
                    && _lastModifiedUtc == modified.toUTC();
            }

            bool equals(qint64 size, QDateTime modified) {
                return _size == size && _lastModifiedUtc == modified.toUTC();
            }

            bool stillValid();
        };

        uint registerHash(const HashID& hash);

        QList<QString> _musicPaths;

        QHash<uint, HashID> _idToHash;
        QHash<HashID, uint> _hashToID;
        QList<HashID> _hashList;

        QHash<HashID, AudioData> _audioCache;
        QMultiHash<HashID, const TagData*> _tagCache;

        QMultiHash<HashID, VerifiedFile*> _filesForHash;
        QHash<QString, VerifiedFile*> _paths;
    };
}
#endif
