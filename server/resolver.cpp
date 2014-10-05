/*
    Copyright (C) 2014, Kevin Andre <hyperquantum@gmail.com>

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

#include "resolver.h"

#include "common/filedata.h"

#include <QFileInfo>
#include <QtDebug>
#include <QtGlobal>

namespace PMP {

    Resolver::Resolver() {
        //qsrand(....);
    }

    void Resolver::registerData(const HashID& hash, const AudioData& data) {
        if (hash.empty()) { return; }

        registerHash(hash);

        AudioData& cached = _audioCache[hash];

        if (data.format() != AudioData::UnknownFormat) {
            cached.setFormat(data.format());
        }

        if (data.trackLength() >= 0) {
            cached.setTrackLength(data.trackLength());
        }
    }

    void Resolver::registerData(const FileData& data) {
        if (data.hash().empty()) { return; }

        registerData(data.hash(), data);
        _tagCache.insert(data.hash(), new TagData(data));
    }

    void Resolver::registerFile(const FileData& file, const QString& filename) {
        if (file.hash().empty()) { return; }

        registerData(file);
        registerFile(file.hash(), filename);
    }

    void Resolver::registerFile(const HashID& hash, const QString& filename) {
        if (hash.empty() || filename.length() <= 0) { return; }

        QFileInfo info(filename);

        if (info.isAbsolute() || info.makeAbsolute()) {
            registerHash(hash);
            _pathCache.insert(hash, info.filePath());
        }
    }

    bool Resolver::haveAnyPathInfo(const HashID& hash) {
        QList<QString> paths = _pathCache.values(hash);
        return paths.count() > 0;
    }

    QString Resolver::findPath(const HashID& hash) {
        QList<QString> paths = _pathCache.values(hash);
        qDebug() << "Resolver::findPath for hash " << hash.dumpToString();
        qDebug() << " candidates for hash: " << paths.count();

        QString path;
        foreach(path, paths) {
            qDebug() << " candidate: " << path;
            QFileInfo info(path);

            if (info.isFile() && info.isReadable()) {
                return path;
            }
        }

        return ""; /* not found */
    }

    const AudioData& Resolver::findAudioData(const HashID& hash) {
        return _audioCache[hash];
    }

    const TagData* Resolver::findTagData(const HashID& hash) {
        QList<const TagData*> tags = _tagCache.values(hash);

        /* try to return a match with complete tags */
        const TagData* result = 0;
        int resultScore = -1;
        const TagData* tag;
        foreach (tag, tags) {
            int score = 0;

            QString title = tag->title();
            QString artist = tag->artist();

            if (title.length() > 0) {
                score += 100000;
                score += 8 * title.length();
            }

            if (artist.length() > 0) {
                score += 80000;
                score += artist.length();
            }

            if (score <= resultScore) { continue; }

            result = tag;
            resultScore = score;
        }

        return result;
    }

    HashID Resolver::getRandom() {
        if (_hashList.empty()) return HashID();

        int randomIndex = qrand() % _hashList.size();
        return _hashList[randomIndex];
    }

    void Resolver::registerHash(const HashID& hash) {
        QHash<HashID, int>::const_iterator it = _hashIndex.constFind(hash);
        if (it != _hashIndex.constEnd()) return; /* registered already */

        _hashList.append(hash);
        _hashIndex.insert(hash, _hashList.size() - 1);
    }

}
