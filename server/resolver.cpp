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

#include "resolver.h"

#include "common/filedata.h"

#include "database.h"

#include <QFileInfo>
#include <QtDebug>
#include <QtGlobal>

namespace PMP {

    Resolver::Resolver() {
        Database* db = Database::instance();

        if (db != 0) {
            QList<QPair<uint, HashID> > hashes = db->getHashes();

            QPair<uint, HashID> pair;
            foreach(pair, hashes) {
                _idToHash[pair.first] = pair.second;
                _hashToID[pair.second] = pair.first;
                _hashList.append(pair.second);
            }

            qDebug() << "loaded" << _hashList.count() << "hashes from the database" << endl;
        }
    }

    uint Resolver::registerHash(const HashID& hash) {
        if (hash.empty()) return 0; /* invalid hash */

        uint id = _hashToID.value(hash, 0);

        if (id > 0) return id; /* registered already */

        Database* db = Database::instance();
        if (db != 0) {
            db->registerHash(hash);
            id = db->getHashID(hash);

            _idToHash[id] = hash;
            _hashToID[hash] = id;
            _hashList.append(hash);

            qDebug() << "got ID" << id << "for registered hash" << hash.dumpToString();
        }

        return id;
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

        registerData(data.hash(), data.audio());
        _tagCache.insert(data.hash(), new TagData(data.tags()));
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
            uint hashID = registerHash(hash);
            _pathCache.insert(hash, info.filePath());
            //qDebug() << "will connect ID" << hashID << "to filename" << filename;

            Database* db = Database::instance();
            if (hashID > 0 && db != 0) {
                db->registerFilename(hashID, info.fileName());
            }
        }
    }

    bool Resolver::haveAnyPathInfo(const HashID& hash) {
        QList<QString> paths = _pathCache.values(hash);
        return !paths.empty();
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

    uint Resolver::getID(const HashID& hash) const {
        return _hashToID.value(hash, 0);
    }

    void Resolver::analysedFile(QString filename, FileData* data) {
        if (data != 0) {
            qDebug() << "file analysis complete:" << filename;
            registerFile(*data, filename);
            delete data;
        }
        else {
            qDebug() << "file analysis FAILED:" << filename;
        }
    }

}
