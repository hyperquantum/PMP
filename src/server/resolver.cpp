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

#include <QDirIterator>
#include <QFileInfo>
#include <QtConcurrent/QtConcurrent>
#include <QtDebug>
#include <QtGlobal>
#include <QThreadPool>

namespace PMP {

    /* ========================== VerifiedFile ========================== */

    bool Resolver::VerifiedFile::stillValid() {
        QFileInfo info(_path);

        if (!info.isFile() || !info.isReadable()
            || !equals(info.size(), info.lastModified()))
        {
            return false;
        }

        return true;
    }

    /* ========================== Resolver ========================== */

    Resolver::Resolver()
     : _lock(QReadWriteLock::Recursive), _fullIndexationWatcher(this)
    {
        connect(
            &_fullIndexationWatcher, &QFutureWatcher<void>::finished,
            this, &Resolver::fullIndexationFinished
        );

        auto db = Database::getDatabaseForCurrentThread();

        if (db != nullptr) {
            QList<QPair<uint, FileHash> > hashes = db->getHashes();

            QPair<uint, FileHash> pair;
            foreach(pair, hashes) {
                _idToHash[pair.first] = pair.second;
                _hashToID[pair.second] = pair.first;
                _hashList.append(pair.second);
            }

            qDebug() << "loaded" << _hashList.count()
                     << "hashes from the database" << endl;
        }
        else {
            qDebug() << "Resolver: could not load hashes because"
                     << "there is no working DB connection" << endl;
        }
    }

    void Resolver::setMusicPaths(QList<QString> paths) {
        QWriteLocker lock(&_lock);
        _musicPaths = paths;
    }

    QList<QString> Resolver::musicPaths() {
        QReadLocker lock(&_lock);
        QList<QString> paths = _musicPaths;
        paths.detach();
        return paths;
    }

    bool Resolver::fullIndexationRunning() {
        return _fullIndexationRunning.load() != 0;
    }

    bool Resolver::startFullIndexation() {
        if (!_fullIndexationRunning.testAndSetAcquire(0, 1)) {
            return false; /* already running */
        }

        emit fullIndexationStarted();
        QFuture<void> future = QtConcurrent::run(this, &Resolver::doFullIndexation);
        _fullIndexationWatcher.setFuture(future);

        return true;
    }

    void Resolver::doFullIndexation() {
        qDebug() << "full indexation started";

        QList<QString> musicPaths = this->musicPaths();

        QList<QString> filesToAnalyze;

        /* traverse filesystem to find music files */
        Q_FOREACH(QString musicPath, musicPaths) {
            QDirIterator it(musicPath, QDirIterator::Subdirectories); /* no symlinks */

            while (it.hasNext()) {
                QFileInfo entry(it.next());
                if (!entry.isFile()) continue;
                if (!FileData::supportsExtension(entry.suffix())) continue;

                filesToAnalyze.append(entry.absoluteFilePath());
            }
        }

        qDebug() << "full indexation:" << filesToAnalyze.size() << "files to analyze";

        /* analyze the files we found */
        Q_FOREACH(QString filePath, filesToAnalyze) {
            QFileInfo info(filePath);

            FileData data = FileData::analyzeFile(filePath);
            if (!data.isValid()) {
                qDebug() << "file analysis FAILED:" << filePath;
                continue;
            }

            registerFile(data, filePath, info.size(), info.lastModified());
        }

        qDebug() << "full indexation finished.";
        _fullIndexationRunning.storeRelease(0);
    }

    uint Resolver::registerHash(const FileHash& hash) {
        if (hash.empty()) return 0; /* invalid hash */

        QWriteLocker lock(&_lock);

        uint id = _hashToID.value(hash, 0);

        if (id > 0) return id; /* registered already */

        auto db = Database::getDatabaseForCurrentThread();
        if (!db) return 0;

        db->registerHash(hash);
        id = db->getHashID(hash);
        if (id <= 0) return 0; /* something went wrong */

        _idToHash[id] = hash;
        _hashToID[hash] = id;
        _hashList.append(hash);

        qDebug() << "got ID" << id << "for registered hash" << hash.dumpToString();

        return id;
    }

    void Resolver::registerData(const FileHash& hash, const AudioData& data) {
        if (hash.empty()) { return; }

        QWriteLocker lock(&_lock);

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

        QWriteLocker lock(&_lock);

        registerData(data.hash(), data.audio());
        _tagCache.insert(data.hash(), new TagData(data.tags()));
    }

    void Resolver::registerFile(const FileData& file, const QString& filename,
                                qint64 fileSize, QDateTime fileLastModified)
    {
        if (file.hash().empty()) { return; }

        QWriteLocker lock(&_lock);

        registerData(file);
        registerFile(file.hash(), filename, fileSize, fileLastModified);
    }

    void Resolver::registerFile(const FileHash& hash, const QString& filename,
                                qint64 fileSize, QDateTime fileLastModified)
    {
        if (hash.empty() || filename.length() <= 0 || fileSize <= 0
            || fileLastModified.isNull())
        {
            return;
        }

        QWriteLocker lock(&_lock);

        uint hashID = registerHash(hash);

        QFileInfo info(filename);
        if (!info.isAbsolute()) return; /* we need absolute names here */

        VerifiedFile* file = _paths.value(filename, 0);
        if (file != 0) {
            if (file->equals(hash, fileSize, fileLastModified))
            {
                /* registered already, and no changes */
                return;
            }

            /* changes found */
            qDebug() << "Resolver: file different now:" << filename;
            _filesForHash.remove(hash, file);
            _paths.remove(filename);
            delete file;
            file = 0;
        }

        file = new VerifiedFile(filename, fileSize, fileLastModified, hash);

        _filesForHash.insert(hash, file);
        _paths[filename] = file;
        //qDebug() << "will connect ID" << hashID << "to filename" << filename;

        auto db = Database::getDatabaseForCurrentThread();
        if (hashID > 0 && db != nullptr) {
            /* save filename without path in the database */
            db->registerFilename(hashID, info.fileName());
        }
    }

    bool Resolver::haveAnyPathInfo(const FileHash& hash) {
        QReadLocker lock(&_lock);

        QList<VerifiedFile*> paths = _filesForHash.values(hash);
        return !paths.empty();
    }

    bool Resolver::pathStillValid(const FileHash& hash, QString path) {
        QWriteLocker lock(&_lock);

        VerifiedFile* file = _paths.value(path, 0);
        if (file) {
            if (file->stillValid()) return true;

            qDebug() << "Resolver::pathStillValid : removing path:" << path;
            _paths.remove(path);
            _filesForHash.remove(hash, file);
            delete file;
            return false;
        }

        Q_FOREACH(file, _filesForHash.values(hash)) {
            if (file->_path != path) continue;

            if (file->stillValid()) return true;
            qDebug() << "Resolver::pathStillValid : removing path:" << path;
            //_paths.remove(path);
            _filesForHash.remove(hash, file);
            delete file;
            return false;
        }

        return false;
    }

    QString Resolver::findPath(const FileHash& hash, bool fast) {
        QWriteLocker lock(&_lock);

        QList<VerifiedFile*> files = _filesForHash.values(hash);
        qDebug() << "Resolver::findPath for hash " << hash.dumpToString();
        qDebug() << " candidates for hash: " << files.count();

        foreach(VerifiedFile* file, files) {
            qDebug() << " candidate: " << file->_path;

            if (file->stillValid()) {
                return file->_path;
            }

            qDebug() << "  candidate not suitable anymore, deleting the path";
            _filesForHash.remove(hash, file);
            _paths.remove(file->_path);
            delete file;
        }

        /* stop here if we had to get a result fast */
        if (fast) return "";

        auto db = Database::getDatabaseForCurrentThread();
        if (!db) {
            return ""; /* database unusable */
        }

        uint hashID = getID(hash);

        QList<QString> filenames = db->getFilenames(hashID);
        if (filenames.empty()) return "";

        qDebug() << " going to see if the file was moved somewhere else";

        Q_FOREACH(QString musicPath, _musicPaths) {
            QDirIterator it(musicPath, QDir::Dirs | QDir::Readable | QDir::NoDotAndDotDot,
                            QDirIterator::Subdirectories);

            while (it.hasNext()) {
                QFileInfo entry(it.next());
                if (!entry.isDir()) continue;

                QDir dir(entry.filePath());

                Q_FOREACH(QString fileShort, filenames) {
                    if (!dir.exists(fileShort)) continue;

                    QString candidatePath = dir.filePath(fileShort);

                    qDebug() << "  checking out:" << candidatePath;

                    QFileInfo candidate(candidatePath);

                    if (_paths.contains(candidatePath)) {
                        continue; /* this one will have a different hash */
                    }

                    FileData d = FileData::analyzeFile(candidate);
                    if (!d.isValid()) continue; /* failed to analyze */

                    registerFile(
                        d, candidatePath, candidate.size(), candidate.lastModified()
                    );

                    if (d.hash() == hash) {
                        qDebug() << "   we have a MATCH!";
                        return candidatePath;
                    }
                }
            }
        }

        return ""; /* not found */
    }

    const AudioData& Resolver::findAudioData(const FileHash& hash) {
        QReadLocker lock(&_lock);
        return _audioCache[hash];
    }

    const TagData* Resolver::findTagData(const FileHash& hash) {
        QReadLocker lock(&_lock);

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

    FileHash Resolver::getRandom() {
        QReadLocker lock(&_lock);
        if (_hashList.empty()) return FileHash();

        int randomIndex = qrand() % _hashList.size();
        return _hashList[randomIndex];
    }

    uint Resolver::getID(const FileHash& hash) {
        QReadLocker lock(&_lock);
        return _hashToID.value(hash, 0);
    }
}
