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
#include <QThreadPool>

namespace PMP {

    /* ========================== private class declarations ========================== */

    struct Resolver::VerifiedFile {
        Resolver::HashKnowledge* _parent;
        QString _path;
        qint64 _size;
        QDateTime _lastModifiedUtc;

        VerifiedFile(Resolver::HashKnowledge* parent, QString path,
                     qint64 size, QDateTime lastModified)
         : _parent(parent), _path(path),
           _size(size), _lastModifiedUtc(lastModified.toUTC())
        {
            //
        }

        void update(qint64 size, QDateTime modified) {
            _size = size;
            _lastModifiedUtc = modified;
        }

        bool equals(FileHash const& hash, qint64 size, QDateTime modified);

        bool equals(qint64 size, QDateTime modified) {
            return _size == size && _lastModifiedUtc == modified.toUTC();
        }

        bool stillValid();
    };

    class Resolver::HashKnowledge {
        Resolver* _parent;
        FileHash _hash;
        uint _hashId;
        AudioData _audio;
        QList<const TagData*> _tags;
        QList<Resolver::VerifiedFile*> _files;
        QString _quickTitle;
        QString _quickArtist;

    public:
        HashKnowledge(Resolver* parent, FileHash hash, uint hashId)
         : _parent(parent), _hash(hash), _hashId(hashId)
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

        TagData const* findBestTag();
        QString quickTitle() { return _quickTitle; }
        QString quickArtist() { return _quickArtist; }

        void addPath(const QString& filename,
                     qint64 fileSize, QDateTime fileLastModified);
        void removeInvalidPath(Resolver::VerifiedFile* file);

        bool isAvailable();
        bool isStillValid(Resolver::VerifiedFile* file);
        QString getFile();
    };

    /* ========================== VerifiedFile ========================== */

    bool Resolver::VerifiedFile::equals(FileHash const& hash,
                                        qint64 size, QDateTime modified)
    {
        return _size == size && _lastModifiedUtc == modified.toUTC()
            && _parent->hash() == hash;
    }

    bool Resolver::VerifiedFile::stillValid() {
        QFileInfo info(_path);

        if (!info.isFile() || !info.isReadable()
            || !equals(info.size(), info.lastModified()))
        {
            return false;
        }

        return true;
    }

    /* ========================== HashKnowledge ========================= */

    void Resolver::HashKnowledge::addAudioInfo(const AudioData& audio) {
        if (audio.format() != AudioData::UnknownFormat) {
            _audio.setFormat(audio.format());
        }

        /* TODO: what if the existing length is different? */
        if (audio.trackLength() >= 0) {
            _audio.setTrackLength(audio.trackLength());
        }
    }

    void Resolver::HashKnowledge::addTags(const TagData *t) {
        if (!t) return;

        /* check for duplicates */
        Q_FOREACH(const TagData* tag, _tags) {
            if (*tag == *t) return; /* a duplicate */
        }

        _tags.append(t);
        auto tags = findBestTag();
        if (tags) {
            QString oldQuickTitle = _quickTitle;
            QString oldQuickArtist = _quickArtist;

            _quickTitle = tags->title();
            _quickArtist = tags->artist();

            if (oldQuickTitle != _quickTitle || oldQuickArtist != _quickArtist)
            {
                emit _parent->hashTagInfoChanged(_hash, _quickTitle, _quickArtist);
            }
        }
    }

    TagData const* Resolver::HashKnowledge::findBestTag() {
        /* try to return a match with complete tags */
        const TagData* result = nullptr;
        int resultScore = -1;
        Q_FOREACH (const TagData* tag, _tags) {
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

    void Resolver::HashKnowledge::addPath(const QString& filename,
                                          qint64 fileSize, QDateTime fileLastModified)
    {
        QFileInfo info(filename);
        if (!info.isAbsolute()) {
            qDebug() << "Resolver: cannot add file without absolute path:" << filename;
            return; /* we need absolute names here */
        }

        /* check if the file was already known */
        VerifiedFile* file = _parent->_paths.value(filename, nullptr);
        if (file) {
            if (file->_parent == this) {
                /* make sure file specifics are up-to-date and that is all */
                file->update(fileSize, fileLastModified);
                return;
            }

            /* previously know file had a different hash */
            qDebug() << "Resolver: file changed into another hash:" << filename;
            file->_parent->removeInvalidPath(file);
            //file = nullptr;
        }

        file = new VerifiedFile(this, filename, fileSize, fileLastModified);
        _files.append(file);
        _parent->_paths[filename] = file;

        // TODO: move this to another thread
        auto db = Database::getDatabaseForCurrentThread();
        if (_hashId > 0 && db) {
            /* save filename without path in the database */
            db->registerFilename(_hashId, info.fileName());
        }

        if (_files.length() == 1) { /* count went from 0 to 1 */
            emit _parent->hashBecameAvailable(_hash);
        }
    }

    void Resolver::HashKnowledge::removeInvalidPath(Resolver::VerifiedFile* file) {
        qDebug() << "Resolver: removing path:" << file->_path;

        if (_parent->_paths.value(file->_path, nullptr) == file) {
            _parent->_paths.remove(file->_path);
        }

        _files.removeOne(file);
        delete file;

        if (_files.length() == 0) { /* count went from 1 to 0 */
            emit _parent->hashBecameUnavailable(_hash);
        }
    }

    bool Resolver::HashKnowledge::isStillValid(Resolver::VerifiedFile* file) {
        if (file->_parent != this) return false;

        if (file->stillValid()) return true;

        removeInvalidPath(file);
        return false;
    }

    bool Resolver::HashKnowledge::isAvailable() {
        Resolver::VerifiedFile* file = nullptr;

        Q_FOREACH(file, _files) {
            if (file->stillValid()) return true;

            removeInvalidPath(file);
        }

        return false;
    }

    QString Resolver::HashKnowledge::getFile() {
        Resolver::VerifiedFile* file = nullptr;

        Q_FOREACH(file, _files) {
            if (file->stillValid()) return file->_path;

            removeInvalidPath(file);
        }

        return ""; /* no file available */
    }

    /* ========================== Resolver ========================== */

    Resolver::Resolver()
     : _lock(QReadWriteLock::Recursive),
       _randomDevice(), _randomEngine(_randomDevice()),
       _fullIndexationRunning(false), _fullIndexationWatcher(this)
    {
        connect(
            &_fullIndexationWatcher, &QFutureWatcher<void>::finished,
            this, &Resolver::onFullIndexationFinished
        );

        auto db = Database::getDatabaseForCurrentThread();

        if (db != nullptr) {
            QList<QPair<uint, FileHash> > hashes = db->getHashes();

            QPair<uint, FileHash> pair;
            foreach(pair, hashes) {
                auto knowledge = new HashKnowledge(this, pair.second, pair.first);
                _hashKnowledge.insert(pair.second, knowledge);
                _idToHash.insert(pair.first, knowledge);
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
        return _fullIndexationRunning;
    }

    bool Resolver::startFullIndexation() {
        if (_fullIndexationRunning) {
            return false; /* already running */
        }

        _fullIndexationRunning = true;
        emit fullIndexationRunStatusChanged(true);
        QFuture<void> future = QtConcurrent::run(this, &Resolver::doFullIndexation);
        _fullIndexationWatcher.setFuture(future);

        return true;
    }

    void Resolver::onFullIndexationFinished() {
        _fullIndexationRunning = false;
        emit fullIndexationRunStatusChanged(false);
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
    }

    Resolver::HashKnowledge* Resolver::registerHash(const FileHash& hash) {
        if (hash.empty()) return nullptr; /* invalid hash */

        QWriteLocker lock(&_lock);

        auto knowledge = _hashKnowledge.value(hash, nullptr);
        if (!knowledge) {
            knowledge = new HashKnowledge(this, hash, 0);
            _hashKnowledge[hash] = knowledge;
            _hashList.append(hash);
        }
        else if (knowledge->id() > 0) {
            return knowledge; /* registered already */
        }

        auto db = Database::getDatabaseForCurrentThread();
        if (!db) return knowledge; /* database not available */

        db->registerHash(hash);
        uint id = db->getHashID(hash);
        if (id <= 0) return knowledge; /* something went wrong */

        knowledge->setId(id);
        _idToHash[id] = knowledge;

        qDebug() << "got ID" << id << "for registered hash" << hash.dumpToString();

        return knowledge;
    }

    Resolver::HashKnowledge* Resolver::registerData(const FileData& data) {
        if (data.hash().empty()) { return nullptr; }

        QWriteLocker lock(&_lock);

        auto knowledge = registerHash(data.hash());
        if (!knowledge) return nullptr; /* something went wrong */

        knowledge->addAudioInfo(data.audio());
        knowledge->addTags(new TagData(data.tags()));

        return knowledge;
    }

    void Resolver::registerFile(const FileData& file, const QString& filename,
                                qint64 fileSize, QDateTime fileLastModified)
    {
        if (file.hash().empty()) { return; }

        QWriteLocker lock(&_lock);

        auto knowledge = registerData(file);
        registerFile(knowledge, filename, fileSize, fileLastModified);
    }

    void Resolver::registerFile(HashKnowledge* hash, const QString& filename,
                                qint64 fileSize, QDateTime fileLastModified)
    {
        if (!hash || filename.length() <= 0 || fileSize <= 0 || fileLastModified.isNull())
        {
            return;
        }

        QWriteLocker lock(&_lock);

        hash->addPath(filename, fileSize, fileLastModified);
    }

    bool Resolver::haveFileFor(const FileHash& hash) {
        QWriteLocker lock(&_lock);

        auto knowledge = _hashKnowledge.value(hash, nullptr);
        return knowledge && knowledge->isAvailable();
    }

    bool Resolver::pathStillValid(const FileHash& hash, QString path) {
        QWriteLocker lock(&_lock);

        VerifiedFile* file = _paths.value(path, nullptr);
        if (!file) return false;

        auto knowledge = file->_parent;
        return knowledge->isStillValid(file);
    }

    QString Resolver::findPath(const FileHash& hash, bool fast) {
        QWriteLocker lock(&_lock);

        qDebug() << "Resolver::findPath for hash " << hash.dumpToString();

        auto knowledge = _hashKnowledge.value(hash, nullptr);
        if (knowledge) {
            QString path = knowledge->getFile();
            if (path.length() > 0) return path;
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

        auto knowledge = _hashKnowledge.value(hash, nullptr);
        if (knowledge) return knowledge->audio();

        return _emptyAudioData; /* we could not register the hash */
    }

    const TagData* Resolver::findTagData(const FileHash& hash) {
        QReadLocker lock(&_lock);

        auto knowledge = _hashKnowledge.value(hash, nullptr);
        if (knowledge) return knowledge->findBestTag();

        return nullptr;
    }

    FileHash Resolver::getRandom() {
        QReadLocker lock(&_lock);
        if (_hashList.empty()) return FileHash();

        std::uniform_int_distribution<int> uniformDistr(0, _hashList.size() - 1);
        int randomIndex = uniformDistr(_randomEngine);
        return _hashList[randomIndex];
    }

    QList<FileHash> Resolver::getAllHashes() {
        QReadLocker lock(&_lock);
        auto copy = _hashList;
        return copy;
    }

    QList<CollectionTrackInfo> Resolver::getHashesTrackInfo(QList<FileHash> hashes) {
        QReadLocker lock(&_lock);

        QList<CollectionTrackInfo> result;
        result.reserve(hashes.size());

        Q_FOREACH(auto hash, hashes) {
            auto knowledge = _hashKnowledge.value(hash, nullptr);
            if (!knowledge) continue;

            CollectionTrackInfo info(hash, knowledge->isAvailable(),
                                     knowledge->quickTitle(), knowledge->quickArtist());

            result.append(info);
        }

        return result;
    }

    uint Resolver::getID(const FileHash& hash) {
        QReadLocker lock(&_lock);

        auto knowledge = _hashKnowledge.value(hash, nullptr);
        if (knowledge) return knowledge->id();

        return 0;
    }
}
