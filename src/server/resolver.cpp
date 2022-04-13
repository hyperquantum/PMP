/*
    Copyright (C) 2014-2022, Kevin Andre <hyperquantum@gmail.com>

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

#include "common/concurrent.h"
#include "common/containerutil.h"
#include "common/fileanalyzer.h"

#include "database.h"

#include <QDirIterator>
#include <QFileInfo>
#include <QSet>
#include <QtConcurrent/QtConcurrent>
#include <QtDebug>
#include <QThreadPool>

#include <algorithm>
#include <limits>

namespace PMP
{
    Future<void, void> HashIdRegistrar::loadAllFromDatabase()
    {
        auto work =
            [this]() -> ResultOrError<void, void>
            {
                auto db = Database::getDatabaseForCurrentThread();
                if (!db) return failure; /* database not available */

                auto hashesResult = db->getHashes();
                if (hashesResult.failed())
                    return failure;

                const auto hashes = hashesResult.result();

                QMutexLocker lock(&_mutex);
                for (auto& pair : hashes)
                {
                    _hashes.insert(pair.second, pair.first);
                    _ids.insert(pair.first, pair.second);
                }

                qDebug() << "loaded" << hashes.count() << "hashes from the database";
                return success;
            };

        Promise<void, void> promise;
        return Concurrent::run<void, void>(std::move(promise), work);
    }

    Future<uint, void> HashIdRegistrar::getOrCreateId(FileHash hash)
    {
        {
            QMutexLocker lock(&_mutex);
            auto id = _hashes.value(hash, 0);
            if (id > 0)
                return Future<uint, void>::fromResult(id);
        }

        auto work =
            [this, hash]() -> ResultOrError<uint, void>
            {
                auto db = Database::getDatabaseForCurrentThread();
                if (!db) return failure; /* database not available */

                db->registerHash(hash);
                uint id = db->getHashID(hash);
                if (id <= 0) return failure; /* something went wrong */

                QMutexLocker lock(&_mutex);
                _hashes.insert(hash, id);
                _ids.insert(id, hash);
                return id;
            };

        Promise<uint, void> promise;
        return Concurrent::run<uint, void>(std::move(promise), work);
    }

    QVector<QPair<uint, FileHash>> HashIdRegistrar::getAllLoaded()
    {
        QMutexLocker lock(&_mutex);

        QVector<QPair<uint, FileHash>> result;
        result.reserve(_ids.size());

        for (auto it = _ids.begin(); it != _ids.end(); ++it)
        {
            result << qMakePair(it.key(), it.value());
        }

        return result;
    }

    /* ========================== private class declarations ========================== */

    struct Resolver::VerifiedFile
    {
        Resolver::HashKnowledge* _parent;
        QString _path;
        qint64 _size;
        QDateTime _lastModifiedUtc;
        uint _indexationNumber;

        VerifiedFile(Resolver::HashKnowledge* parent, QString path,
                     qint64 size, QDateTime lastModified, uint indexationNumber)
         : _parent(parent), _path(path),
           _size(size), _lastModifiedUtc(lastModified.toUTC()),
           _indexationNumber(indexationNumber)
        {
            //
        }

        void update(qint64 size, QDateTime modified, uint indexationNumber)
        {
            _size = size;
            _lastModifiedUtc = modified;

            if (indexationNumber > 0)
                _indexationNumber = indexationNumber;
        }

        bool equals(FileHash const& hash, qint64 size, QDateTime modified) const;

        bool equals(qint64 size, QDateTime modified) const
        {
            return _size == size && _lastModifiedUtc == modified.toUTC();
        }

        bool hasIndexationNumber(uint indexationNumber) const
        {
            return _indexationNumber == indexationNumber;
        }

        bool stillValid();
    };

    class Resolver::HashKnowledge
    {
        Resolver* _parent;
        FileHash _hash;
        uint _hashId;
        AudioData _audio;
        QList<const TagData*> _tags;
        QList<Resolver::VerifiedFile*> _files;
        QString _quickTitle;
        QString _quickArtist;
        QString _quickAlbum;

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

        qint32 lengthInMilliseconds() { return (qint32)_audio.trackLengthMilliseconds(); }

        void addInfo(AudioData const& audio, TagData const& t);

        TagData const* findBestTag();
        QString quickTitle() { return _quickTitle; }
        QString quickArtist() { return _quickArtist; }
        QString quickAlbum() { return _quickAlbum; }

        void addPath(const QString& filename,
                     qint64 fileSize, QDateTime fileLastModified, uint indexationNumber);
        void removeInvalidPath(Resolver::VerifiedFile* file);

        bool isAvailable();
        bool isStillValid(Resolver::VerifiedFile* file);
        QString getFile();
    };

    /* ========================== VerifiedFile ========================== */

    bool Resolver::VerifiedFile::equals(FileHash const& hash,
                                        qint64 size, QDateTime modified) const
    {
        return _size == size && _lastModifiedUtc == modified.toUTC()
            && _parent->hash() == hash;
    }

    bool Resolver::VerifiedFile::stillValid()
    {
        QFileInfo info(_path);

        if (!info.isFile() || !info.isReadable()
            || !equals(info.size(), info.lastModified()))
        {
            return false;
        }

        return true;
    }

    /* ========================== HashKnowledge ========================= */

    void Resolver::HashKnowledge::addInfo(AudioData const& audio, TagData const& t)
    {
        if (audio.format() != AudioData::UnknownFormat)
        {
            _audio.setFormat(audio.format());
        }

        bool lengthChanged = false;
        auto newLength = audio.trackLengthMilliseconds();
        auto oldLength = _audio.trackLengthMilliseconds();
        if (newLength >= 0 && newLength <= std::numeric_limits<qint32>::max()
                && newLength != oldLength)
        {
            _audio.setTrackLengthMilliseconds(newLength);
            lengthChanged = true;
        }

        /* check for duplicate tags */
        bool tagIsNew = true;
        for (auto* existing : qAsConst(_tags))
        {
            if (existing->title() == t.title()
                    && existing->artist() == t.artist()
                    && existing->album() == t.album())
            {
                tagIsNew = false;
                break;
            }
        }

        bool quickTagsChanged = false;
        if (tagIsNew)
        {
            _tags.append(new TagData(t));

            auto tags = findBestTag();
            if (tags)
            {
                QString oldQuickTitle = _quickTitle;
                QString oldQuickArtist = _quickArtist;
                QString oldQuickAlbum = _quickAlbum;

                _quickTitle = tags->title();
                _quickArtist = tags->artist();
                _quickAlbum = tags->album();

                quickTagsChanged =
                        oldQuickTitle != _quickTitle
                            || oldQuickArtist != _quickArtist
                            || oldQuickAlbum != _quickAlbum;

                if (_tags.size() > 1 && !quickTagsChanged)
                {
                    qDebug() << "extra tag added for track but not switching away from"
                             << " old tag info: title:" << oldQuickTitle
                             << "; artist:" << oldQuickArtist
                             << "; album:" << oldQuickAlbum;
                }
            }
        }

        if (lengthChanged || quickTagsChanged)
        {
            Q_EMIT _parent->hashTagInfoChanged(_hash, _quickTitle, _quickArtist,
                                               _quickAlbum,
                                               _audio.trackLengthMilliseconds());
        }
    }

    TagData const* Resolver::HashKnowledge::findBestTag()
    {
        /* try to return a match with complete tags */
        const TagData* bestTag = nullptr;
        int bestScore = -1;
        for (const TagData* tag : qAsConst(_tags))
        {
            int score = 0;

            int titleLength = tag->title().length();
            int artistLength = tag->artist().length();
            int albumLength = tag->album().length();

            if (titleLength > 0)
            {
                score += 100000;
                score += 8 * std::min(titleLength, 256);
            }

            if (artistLength > 0)
            {
                score += 80000;
                score += std::min(artistLength, 256);
            }

            if (albumLength > 0)
            {
                score += 6000;
                score += std::min(albumLength, 256);
            }

            /* for equal scores we'll use the latest tag */
            if (score >= bestScore)
            {
                bestTag = tag;
                bestScore = score;
            }
        }

        return bestTag;
    }

    void Resolver::HashKnowledge::addPath(const QString& filename,
                                          qint64 fileSize, QDateTime fileLastModified,
                                          uint indexationNumber)
    {
        QFileInfo info(filename);
        if (!info.isAbsolute())
        {
            qDebug() << "Resolver: cannot add file without absolute path:" << filename;
            return; /* we need absolute names here */
        }

        /* check if the file was already known */
        VerifiedFile* file = _parent->_paths.value(filename, nullptr);
        if (file)
        {
            if (file->_parent == this)
            {
                /* make sure file specifics are up-to-date and that is all */
                file->update(fileSize, fileLastModified, indexationNumber);
                return;
            }

            /* previously know file had a different hash */
            qDebug() << "Resolver: file changed into another hash:" << filename;
            file->_parent->removeInvalidPath(file);
            //file = nullptr;
        }

        file = new VerifiedFile(this, filename, fileSize, fileLastModified,
                                indexationNumber);
        _files.append(file);
        _parent->_paths[filename] = file;

        // TODO: move this to another thread
        auto db = Database::getDatabaseForCurrentThread();
        if (_hashId > 0 && db)
        {
            /* save filename without path in the database */
            db->registerFilename(_hashId, info.fileName());

            db->registerFileSize(_hashId, fileSize);
        }

        if (_files.length() == 1) /* count went from 0 to 1 */
        {
            Q_EMIT _parent->hashBecameAvailable(_hash);
        }
    }

    void Resolver::HashKnowledge::removeInvalidPath(Resolver::VerifiedFile* file)
    {
        qDebug() << "Resolver: removing path:" << file->_path;

        if (_parent->_paths.value(file->_path, nullptr) == file)
        {
            _parent->_paths.remove(file->_path);
        }

        _files.removeOne(file);
        delete file;

        if (_files.length() == 0) /* count went from 1 to 0 */
        {
            Q_EMIT _parent->hashBecameUnavailable(_hash);
        }
    }

    bool Resolver::HashKnowledge::isStillValid(Resolver::VerifiedFile* file)
    {
        if (file->_parent != this) return false;

        if (file->stillValid()) return true;

        removeInvalidPath(file);
        return false;
    }

    bool Resolver::HashKnowledge::isAvailable()
    {
        for (auto file : qAsConst(_files))
        {
            if (file->stillValid()) return true;

            removeInvalidPath(file);
        }

        return false;
    }

    QString Resolver::HashKnowledge::getFile()
    {
        for (auto file : qAsConst(_files))
        {
            if (file->stillValid()) return file->_path;

            removeInvalidPath(file);
        }

        return ""; /* no file available */
    }

    /* ========================== Resolver ========================== */

    Resolver::Resolver()
     : _analyzer(new Analyzer(this)),
       _lock(QMutex::Recursive),
       _fullIndexationNumber(1),
       _fullIndexationStatus(FullIndexationStatus::NotRunning)
    {
        connect(_analyzer, &Analyzer::fileAnalysisFailed,
                this, &Resolver::onFileAnalysisFailed);
        connect(_analyzer, &Analyzer::fileAnalysisCompleted,
                this, &Resolver::onFileAnalysisCompleted);
        connect(_analyzer, &Analyzer::finished,
                this, &Resolver::onAnalyzerFinished);

        auto dbLoadingFuture = _hashIdRegistrar.loadAllFromDatabase();
        dbLoadingFuture.addSuccessListener(
            this,
            [this]()
            {
                qDebug() << "Resolver: successfully loaded hashes from the database";

                auto allHashes = _hashIdRegistrar.getAllLoaded();

                uint newHashesCount = 0;
                QMutexLocker lock(&_lock);
                for (auto& pair : allHashes)
                {
                    if (_idToHash.contains(pair.first))
                        continue;

                    auto knowledge = new HashKnowledge(this, pair.second, pair.first);
                    _hashKnowledge.insert(pair.second, knowledge);
                    _idToHash.insert(pair.first, knowledge);
                    _hashList.append(pair.second);
                    newHashesCount++;
                }

                qDebug() << "Resolver: hashes processed; got"
                         << newHashesCount << "new hashes";
            }
        );
        dbLoadingFuture.addFailureListener(
            this,
            []()
            {
                qDebug() << "Resolver: could not load hashes from the database";
            }
        );
    }

    void Resolver::setMusicPaths(QStringList paths)
    {
        QMutexLocker lock(&_lock);
        _musicPaths = paths;

        qDebug() << "music paths set to:" << paths.join("; ");
    }

    QStringList Resolver::musicPaths()
    {
        QMutexLocker lock(&_lock);
        QStringList paths = _musicPaths;
        paths.detach();
        return paths;
    }

    bool Resolver::fullIndexationRunning()
    {
        return _fullIndexationStatus != FullIndexationStatus::NotRunning;
    }

    bool Resolver::startFullIndexation()
    {
        if (fullIndexationRunning())
            return false; /* already running */

        qDebug() << "full indexation starting";
        _fullIndexationNumber += 2; /* add 2 so it will never become zero */
        _fullIndexationStatus = FullIndexationStatus::FileSystemTraversal;
        Q_EMIT fullIndexationRunStatusChanged(true);
        QtConcurrent::run(this, &Resolver::doFullIndexationFileSystemTraversal);

        return true;
    }

    void Resolver::onFullIndexationFinished()
    {
        Q_EMIT fullIndexationRunStatusChanged(false);
    }

    void Resolver::onFileAnalysisFailed(QString path)
    {
        // TODO
    }

    void Resolver::onFileAnalysisCompleted(QString path, FileAnalysis analysis)
    {
        QMutexLocker lock(&_lock);

        auto hashes = analysis.hashes();

        HashKnowledge* knowledge;
        if (!hashes.multipleHashes())
        {
            knowledge = registerHash(hashes.main());
        }
        else
        {
            knowledge = nullptr;
            for (auto const& hash : hashes.allHashes())
            {
                auto k = _hashKnowledge.value(hash, nullptr);
                if (!k)
                    continue;

                if (!knowledge)
                    knowledge = k;
                else
                    qDebug() << "detected multiple hashes in use for file" << path << ":"
                             << knowledge->hash() << "and" << k->hash();
            }

            if (!knowledge)
            {
                knowledge = registerHash(hashes.main());
            }
        }

        if (!knowledge)
        {
            qDebug() << "failed to register hash for:" << path;
            return;
        }

        knowledge->addInfo(analysis.audioData(), analysis.tagData());

        auto fileInfo = analysis.fileInfo();

        if (fileInfo.path().length() <= 0
                || fileInfo.size() <= 0
                || fileInfo.lastModifiedUtc().isNull())
        {
            qDebug() << "cannot register file details for:" << path;
            return;
        }

        knowledge->addPath(fileInfo.path(), fileInfo.size(), fileInfo.lastModifiedUtc(),
                           _fullIndexationNumber);
    }

    void Resolver::onAnalyzerFinished()
    {
        if (_fullIndexationStatus
                                == FullIndexationStatus::WaitingForFileAnalysisCompletion)
        {
            _fullIndexationStatus = FullIndexationStatus::CheckingForFileRemovals;
            QtConcurrent::run(this, &Resolver::doFullIndexationCheckForFileRemovals);
        }
    }

    Future<FileHash, void> Resolver::analyzeAndRegisterFileAsync(QString filename)
    {
        auto analysisFuture = _analyzer->analyzeFileAsync(filename);

        auto result =
            analysisFuture.transformResult<FileHash>(
                this,
                [this, filename](FileAnalysis analysis)
                {
                    onFileAnalysisCompleted(filename, analysis);
                    return analysis.hashes().main();
                }
            );

        return result;
    }

    void Resolver::doFullIndexationFileSystemTraversal()
    {
        qDebug() << "full indexation: running file system traversal (music paths)";

        auto musicPaths = this->musicPaths();

        uint fileCount = 0;
        for (QString const& musicPath : qAsConst(musicPaths))
        {
            QDirIterator it(musicPath, QDirIterator::Subdirectories); /* no symlinks */

            while (it.hasNext())
            {
                QFileInfo entry(it.next());
                if (!FileAnalyzer::isFileSupported(entry)) continue;

                fileCount++;
                _analyzer->enqueueFile(entry.absoluteFilePath());
            }
        }

        qDebug() << "full indexation:" << fileCount << "files added to analysis queue";

        if (_analyzer->isFinished())
        {
            _fullIndexationStatus = FullIndexationStatus::CheckingForFileRemovals;
            QtConcurrent::run(this, &Resolver::doFullIndexationCheckForFileRemovals);
        }
        else
        {
            _fullIndexationStatus =
                    FullIndexationStatus::WaitingForFileAnalysisCompletion;
        }
    }

    void Resolver::doFullIndexationCheckForFileRemovals()
    {
        qDebug() << "full indexation: checking for files that have disappeared";

        auto pathsToCheck = getPathsThatDontMatchCurrentFullIndexationNumber();
        qDebug() << "full indexation:" << pathsToCheck.size()
                 << "files need checking for their possible disappearance";

        for (QString const& path : qAsConst(pathsToCheck))
        {
            checkFileStillExistsAndIsValid(path);
        }

        _fullIndexationStatus = FullIndexationStatus::NotRunning;
        qDebug() << "full indexation finished.";
        QTimer::singleShot(0, this, [this]() { onFullIndexationFinished(); });
    }

    Resolver::HashKnowledge* Resolver::registerHash(const FileHash& hash)
    {
        if (hash.isNull()) return nullptr; /* invalid hash */

        QMutexLocker lock(&_lock);

        auto knowledge = _hashKnowledge.value(hash, nullptr);
        if (!knowledge)
        {
            knowledge = new HashKnowledge(this, hash, 0);
            _hashKnowledge[hash] = knowledge;
            _hashList.append(hash);
        }
        else if (knowledge->id() > 0)
        {
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

    QVector<QString> Resolver::getPathsThatDontMatchCurrentFullIndexationNumber()
    {
        QMutexLocker lock(&_lock);

        QVector<QString> result;

        for (auto file : qAsConst(_paths))
        {
            if (!file->hasIndexationNumber(_fullIndexationNumber))
            {
                result << file->_path;
            }
        }

        return result;
    }

    FileHash Resolver::analyzeAndRegisterFileInternal(const QString& filename)
    {
        QFileInfo info(filename);

        FileAnalyzer analyzer(info);
        analyzer.analyze();

        if (!analyzer.analysisDone())
            return FileHash(); /* something went wrong, return invalid hash */

        if (analyzer.audioData().trackLengthMilliseconds()
                > std::numeric_limits<qint32>::max())
        {
            qDebug() << "audio too long, not registering:" << filename;
            return FileHash(); /* file too long, probably not music anyway */
        }

        auto fileSize = info.size();
        auto fileLastModified = info.lastModified();

        FileHash finalHash = analyzer.hash();
        FileHash legacyHash = analyzer.legacyHash();

        QMutexLocker lock(&_lock);

        HashKnowledge* knowledge;
        if (legacyHash.isNull())
        {
            knowledge = registerHash(finalHash);
        }
        else if (!_hashKnowledge.value(legacyHash, nullptr))
        {
            knowledge = registerHash(finalHash);
        }
        else if (!_hashKnowledge.value(finalHash, nullptr))
        {
            qDebug() << "registering file under legacy hash:" << info.fileName();
            knowledge = registerHash(legacyHash);
        }
        else
        {
            qDebug() << "registering file under final hash, but legacy is present:"
                     << info.fileName();
            knowledge = registerHash(finalHash);
        }

        if (!knowledge)
        {
            qDebug() << "failed to register hash for:" << info.fileName();
            return FileHash(); /* something went wrong, return invalid hash */
        }

        knowledge->addInfo(analyzer.audioData(), analyzer.tagData());

        if (filename.length() <= 0 || fileSize <= 0 || fileLastModified.isNull())
        {
            qDebug() << "failed to register file details for:" << info.fileName();
            return FileHash(); /* return invalid hash */
        }

        knowledge->addPath(filename, fileSize, fileLastModified, 0);

        return knowledge->hash();
    }

    bool Resolver::haveFileForHash(const FileHash& hash)
    {
        QMutexLocker lock(&_lock);

        auto knowledge = _hashKnowledge.value(hash, nullptr);
        return knowledge && knowledge->isAvailable();
    }

    bool Resolver::pathStillValid(const FileHash& hash, QString path)
    {
        QMutexLocker lock(&_lock);

        VerifiedFile* file = _paths.value(path, nullptr);
        if (!file) return false;

        auto knowledge = file->_parent;
        if (knowledge->hash() != hash) return false;

        return knowledge->isStillValid(file);
    }

    Nullable<FileHash> Resolver::findHashForFilePath(QString path)
    {
        QMutexLocker lock(&_lock);

        VerifiedFile* file = _paths.value(path, nullptr);
        if (file)
        {
            auto hash = file->_parent->hash();
            return hash;
        }

        qDebug() << "path is not yet registered, putting it in the analyzer queue:"
                 << path;
        _analyzer->enqueueFile(path);
        return null;
    }

    void Resolver::checkFileStillExistsAndIsValid(QString path)
    {
        QMutexLocker lock(&_lock);

        VerifiedFile* file = _paths.value(path, nullptr);
        if (!file) return;

        auto knowledge = file->_parent;
        (void)knowledge->isStillValid(file);
    }

    QString Resolver::findPathForHash(const FileHash& hash, bool fast)
    {
        QMutexLocker lock(&_lock);

        qDebug() << "Resolver::findPathForHash called for hash" << hash;

        auto knowledge = _hashKnowledge.value(hash, nullptr);
        if (knowledge)
        {
            QString path = knowledge->getFile();
            if (!path.isEmpty())
            {
                qDebug() << " returning path that was already known:" << path;
                return path;
            }
        }

        /* stop here if we had to get a result quickly */
        if (fast)
        {
            qDebug() << " no precomputed path available; we give up because no time left";
            return {};
        }

        auto db = Database::getDatabaseForCurrentThread();
        if (!db)
        {
            qDebug() << " database not available";
            return {}; /* database unusable */
        }

        uint hashId = getID(hash);
        if (!hashId)
        {
            qDebug() << " hash not in database, cannot find path.";
            return {};
        }

        qDebug() << " going to see if the file was moved somewhere else";

        auto path = findPathForHashByLikelyFilename(*db, hash, hashId);
        if (!path.isEmpty())
        {
            qDebug() << " found match by filename heuristic:" << path;
            return path;
        }

        path = findPathByQuickScanForNewFiles(*db, hash, hashId);
        if (!path.isEmpty())
        {
            qDebug() << " found match by quick scan for new files:" << path;
            return path;
        }

        qDebug() << " could not easily locate a file that matches the hash; giving up";
        return {}; /* not found */
    }

    QString Resolver::findPathForHashByLikelyFilename(Database& db, const FileHash& hash,
                                                      uint hashId)
    {
        auto filenamesResult = db.getFilenames(hashId);
        if (filenamesResult.failed()) /* no known filenames */
            return {};

        auto const filenames = filenamesResult.result();

        qDebug() << " going to look for a matching file using a filename-based heuristic";

        for (QString const& musicPath : qAsConst(_musicPaths))
        {
            QDirIterator it(musicPath, QDir::Dirs | QDir::Readable | QDir::NoDotAndDotDot,
                            QDirIterator::Subdirectories);

            while (it.hasNext())
            {
                QFileInfo entry(it.next());
                if (!entry.isDir()) continue;

                QDir dir(entry.filePath());

                for (QString const& fileShort : filenames)
                {
                    if (!dir.exists(fileShort)) continue;

                    QString candidatePath = dir.filePath(fileShort);

                    qDebug() << "  checking out:" << candidatePath;

                    if (_paths.contains(candidatePath))
                    {
                        qDebug() << "  ignoring:" << candidatePath;
                        continue; /* this one will have a different hash */
                    }

                    auto candidateHash = analyzeAndRegisterFileInternal(candidatePath);
                    if (candidateHash.isNull()) continue; /* failed to analyze */

                    if (candidateHash == hash)
                        return candidatePath;
                }
            }
        }

        return {}; /* cannot find a matching file based on name alone */
    }

    QString Resolver::findPathByQuickScanForNewFiles(Database& db, const FileHash& hash,
                                                     uint hashId)
    {
        /* get likely file sizes */
        auto previousFileSizesResult = db.getFileSizes(hashId);
        QSet<qint64> previousFileSizes;
        if (previousFileSizesResult.succeeded())
            ContainerUtil::addToSet(previousFileSizesResult.result(), previousFileSizes);

        auto musicPaths = this->musicPaths();

        QVector<QString> newFilesToScan;

        qDebug() << " going to look for a matching file using a filesize-based heuristic";

        /* traverse filesystem to find music files */
        for (QString const& musicPath : qAsConst(musicPaths))
        {
            QDirIterator it(musicPath, QDirIterator::Subdirectories); /* no symlinks */

            while (it.hasNext())
            {
                QFileInfo entry(it.next());
                if (!FileAnalyzer::isFileSupported(entry)) continue;

                auto candidatePath = entry.absoluteFilePath();

                if (_paths.contains(candidatePath))
                    continue; /* this file is already indexed */

                if (!previousFileSizes.contains(entry.size()))
                {   /* file size does not indicate a match */
                    newFilesToScan.append(candidatePath);
                    continue;
                }

                qDebug() << "  checking out file with matching size:" << candidatePath;

                auto candidateHash = analyzeAndRegisterFileInternal(candidatePath);
                if (candidateHash == hash)
                    return candidatePath;
            }
        }

        return findPathByQuickScanOfNewFiles(newFilesToScan, hash);
    }

    QString Resolver::findPathByQuickScanOfNewFiles(QVector<QString> newFiles,
                                                    const FileHash& hash)
    {
        const int maxNewFilesToScan = 3;

        qDebug() << " going to look for a matching file by examining new files";

        for (int i = 0; i < newFiles.size(); ++i)
        {
            if (i >= maxNewFilesToScan)
            {
                qDebug() << "  reached maximum number of new files to scan; stopping now";
                // TODO: put indexation of the remaining files in some kind of work queue
                break;
            }

            auto newFilePath = newFiles[i];
            qDebug() << "  checking out new file:" << newFilePath;

            auto candidateHash = analyzeAndRegisterFileInternal(newFilePath);
            if (candidateHash == hash)
                return newFilePath;
        }

        return {}; /* found nothing */
    }

    const AudioData& Resolver::findAudioData(const FileHash& hash)
    {
        QMutexLocker lock(&_lock);

        auto knowledge = _hashKnowledge.value(hash, nullptr);
        if (knowledge) return knowledge->audio();

        return _emptyAudioData; /* we could not register the hash */
    }

    const TagData* Resolver::findTagData(const FileHash& hash)
    {
        QMutexLocker lock(&_lock);

        auto knowledge = _hashKnowledge.value(hash, nullptr);
        if (knowledge) return knowledge->findBestTag();

        return nullptr;
    }

    QVector<FileHash> Resolver::getAllHashes()
    {
        QMutexLocker lock(&_lock);
        auto copy = _hashList.toVector();
        return copy;
    }

    QVector<CollectionTrackInfo> Resolver::getHashesTrackInfo(QVector<FileHash> hashes)
    {
        QMutexLocker lock(&_lock);

        QVector<CollectionTrackInfo> result;
        result.reserve(hashes.size());

        for (auto& hash : qAsConst(hashes))
        {
            auto knowledge = _hashKnowledge.value(hash, nullptr);
            if (!knowledge) continue;

            auto lengthInMilliseconds = knowledge->audio().trackLengthMilliseconds();
            if (lengthInMilliseconds > std::numeric_limits<qint32>::max())
            {
                lengthInMilliseconds = 0;
            }

            CollectionTrackInfo info(hash, knowledge->isAvailable(),
                                     knowledge->quickTitle(), knowledge->quickArtist(),
                                     knowledge->quickAlbum(),
                                     (qint32)lengthInMilliseconds);

            result.append(info);
        }

        return result;
    }

    FileHash Resolver::getHashByID(uint id)
    {
        QMutexLocker lock(&_lock);

        auto knowledge = _idToHash.value(id, nullptr);
        if (knowledge) return knowledge->hash();

        qWarning() << "Resolver::getHashByID: ID" << id << "not found";

        return FileHash(); /* empty (invalid) hash */
    }

    uint Resolver::getID(const FileHash& hash)
    {
        QMutexLocker lock(&_lock);

        auto knowledge = _hashKnowledge.value(hash, nullptr);
        if (knowledge) return knowledge->id();

        return 0;
    }

    QList<QPair<uint, FileHash>> Resolver::getIDs(QList<FileHash> hashes)
    {
        QMutexLocker lock(&_lock);

        QList<QPair<uint, FileHash>> result;
        result.reserve(hashes.size());

        for (auto& hash : qAsConst(hashes))
        {
            auto knowledge = _hashKnowledge.value(hash, nullptr);
            if (!knowledge) continue;

            result.append(QPair<uint, FileHash>(knowledge->id(), hash));
        }

        return result;
    }

    QVector<QPair<uint, FileHash>> Resolver::getIDs(QVector<FileHash> hashes)
    {
        QMutexLocker lock(&_lock);

        QVector<QPair<uint, FileHash>> result;
        result.reserve(hashes.size());

        for (auto const& hash : qAsConst(hashes))
        {
            auto knowledge = _hashKnowledge.value(hash, nullptr);
            if (!knowledge) continue;

            result.append(QPair<uint, FileHash>(knowledge->id(), hash));
        }

        return result;
    }
}
