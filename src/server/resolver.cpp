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

#include "resolver.h"

#include "common/async.h"
#include "common/concurrent.h"
#include "common/fileanalyzer.h"

#include "analyzer.h"
#include "database.h"
#include "filefinder.h"
#include "hashidregistrar.h"
#include "hashrelations.h"
#include "historystatistics.h"

#include <QDirIterator>
#include <QFileInfo>
#include <QSet>
#include <QtConcurrent/QtConcurrent>
#include <QtDebug>
#include <QThreadPool>

#include <algorithm>
#include <limits>

namespace PMP::Server
{

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
        Promise<SuccessType, FailureType> _promiseForFirstFileAnalyzed;
        AudioData _audio;
        QList<const TagData*> _tags;
        QList<Resolver::VerifiedFile*> _files;
        QString _quickTitle;
        QString _quickArtist;
        QString _quickAlbum;
        QString _quickAlbumArtist;
        bool _firstFileAnalyzed { false };

    public:
        HashKnowledge(Resolver* parent, FileHash hash, uint hashId)
         : _parent(parent), _hash(hash), _hashId(hashId),
            _promiseForFirstFileAnalyzed(Async::createPromise<SuccessType, FailureType>())
        {
            //
        }

        const FileHash& hash() const { return _hash; }

        uint id() const { return _hashId; }
        void setId(uint id) { _hashId = id; }

        Future<SuccessType, FailureType> getFutureForFirstFileAnalyzed() const
        {
            return _promiseForFirstFileAnalyzed.future();
        }

        void markFileAnalyzed()
        {
            if (_firstFileAnalyzed)
                return;

            _firstFileAnalyzed = true;
            _promiseForFirstFileAnalyzed.setOutcome(success);
        }

        const AudioData& audio() const { return _audio; }
        AudioData& audio() { return _audio; }

        qint32 lengthInMilliseconds() { return (qint32)_audio.trackLengthMilliseconds(); }

        void addInfo(AudioData const& audio, TagData const& t);

        TagData const* findBestTag();
        QString quickTitle() { return _quickTitle; }
        QString quickArtist() { return _quickArtist; }
        QString quickAlbum() { return _quickAlbum; }
        QString quickAlbumArtist() { return _quickAlbumArtist; }

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
                    && existing->album() == t.album()
                    && existing->albumArtist() == t.albumArtist())
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
                auto oldQuickTitle = _quickTitle;
                auto oldQuickArtist = _quickArtist;
                auto oldQuickAlbum = _quickAlbum;
                auto oldQuickAlbumArtist = _quickAlbumArtist;

                _quickTitle = tags->title();
                _quickArtist = tags->artist();
                _quickAlbum = tags->album();
                _quickAlbumArtist = tags->albumArtist();

                quickTagsChanged =
                        oldQuickTitle != _quickTitle
                            || oldQuickArtist != _quickArtist
                            || oldQuickAlbum != _quickAlbum
                            || oldQuickAlbumArtist != _quickAlbumArtist;

                if (_tags.size() > 1 && !quickTagsChanged)
                {
                    qDebug() << "extra tag added for track but not switching away from"
                             << " old tag info: title:" << oldQuickTitle
                             << "; artist:" << oldQuickArtist
                             << "; album:" << oldQuickAlbum
                             << "; album artist:" << oldQuickAlbumArtist;
                }
            }
        }

        if (lengthChanged || quickTagsChanged)
        {
            Q_EMIT _parent->hashTagInfoChanged(_hash, _quickTitle, _quickArtist,
                                               _quickAlbum, _quickAlbumArtist,
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
            int albumArtistLength = tag->albumArtist().length();

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

            if (albumArtistLength > 0)
            {
                score += 1000;
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
        VerifiedFile* file = _parent->_pathToVerifiedFile.value(filename, nullptr);
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
        _parent->_pathToVerifiedFile[filename] = file;

        // TODO: move this to another thread
        auto db = Database::getDatabaseForCurrentThread();
        if (_hashId > 0 && db)
        {
            int currentYear = QDateTime::currentDateTimeUtc().date().year();

            /* save filename without path in the database */
            db->registerFilenameSeen(_hashId, info.fileName(), currentYear);

            db->registerFileSizeSeen(_hashId, fileSize, currentYear);
        }

        if (_files.length() == 1) /* count went from 0 to 1 */
        {
            Q_EMIT _parent->hashBecameAvailable(_hash);
        }
    }

    void Resolver::HashKnowledge::removeInvalidPath(Resolver::VerifiedFile* file)
    {
        qDebug() << "Resolver: removing path:" << file->_path;

        _parent->_fileLocations.remove(_hashId, file->_path);

        if (_parent->_pathToVerifiedFile.value(file->_path, nullptr) == file)
        {
            _parent->_pathToVerifiedFile.remove(file->_path);
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

        return {}; /* no file available */
    }

    /* ========================== Resolver ========================== */

    Resolver::Resolver(HashIdRegistrar* hashIdRegistrar, HashRelations* hashRelations,
                       HistoryStatistics* historyStatistics)
     : _hashIdRegistrar(hashIdRegistrar),
       _hashRelations(hashRelations),
       _historyStatistics(historyStatistics),
       _fullIndexationNumber(1)
    {
        _analyzer = new Analyzer(this);
        _fileFinder = new FileFinder(this, _hashIdRegistrar, &_fileLocations, _analyzer);

        connect(_analyzer, &Analyzer::fileAnalysisFailed,
                this, &Resolver::onFileAnalysisFailed);
        connect(_analyzer, &Analyzer::fileAnalysisCompleted,
                this, &Resolver::onFileAnalysisCompleted);
        connect(_analyzer, &Analyzer::finished,
                this, &Resolver::onAnalyzerFinished);

        auto dbLoadingFuture = _hashIdRegistrar->loadAllFromDatabase();
        dbLoadingFuture.handleOnEventLoop(
            this,
            [this](SuccessOrFailure outcome)
            {
                if (outcome.failed())
                {
                    qWarning() << "Resolver: could not load hashes from the database";
                    return;
                }

                qDebug() << "Resolver: successfully loaded hashes from the database";

                auto allHashes = _hashIdRegistrar->getAllLoaded();

                uint newHashesCount = 0;
                QMutexLocker lock(&_lock);
                for (auto& pair : allHashes)
                {
                    if (_idToKnowledge.contains(pair.first))
                        continue;

                    auto knowledge = new HashKnowledge(this, pair.second, pair.first);
                    _hashToKnowledge.insert(pair.second, knowledge);
                    _idToKnowledge.insert(pair.first, knowledge);
                    _hashesList.append(pair.second);
                    newHashesCount++;
                }

                qDebug() << "Resolver: hashes processed; got"
                         << newHashesCount << "new hashes";
            }
        );
    }

    void Resolver::setMusicPaths(QStringList paths)
    {
        _fileFinder->setMusicPaths(paths);

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

    bool Resolver::isFullIndexationRunning()
    {
        return _fullIndexationStatus != FullIndexationStatus::NotRunning;
    }

    bool Resolver::isQuickScanForNewFilesRunning()
    {
        return _quickScanStatus != QuickScanForNewFilesStatus::NotRunning;
    }

    Result Resolver::startFullIndexation()
    {
        if (isFullIndexationRunning() || isQuickScanForNewFilesRunning())
            return Error::operationAlreadyRunning();

        qDebug() << "starting full indexation";

        _fullIndexationNumber += 2; /* add 2 so it will never become zero */
        _fullIndexationStatus = FullIndexationStatus::FileSystemTraversal;
        Q_EMIT fullIndexationRunStatusChanged();

        QtConcurrent::run(this, &Resolver::doFullIndexationFileSystemTraversal);

        return Success();
    }

    Result Resolver::startQuickScanForNewFiles()
    {
        if (isFullIndexationRunning() || isQuickScanForNewFilesRunning())
            return Error::operationAlreadyRunning();

        qDebug() << "starting quick scan for new files";

        _quickScanStatus = QuickScanForNewFilesStatus::FileSystemTraversal;
        Q_EMIT quickScanForNewFilesRunStatusChanged();

        QtConcurrent::run(this, &Resolver::doQuickScanForNewFilesFileSystemTraversal);

        return Success();
    }

    void Resolver::onQuickScanForNewFilesFinished()
    {
        qDebug() << "quick scan for new files finished";

        Q_EMIT quickScanForNewFilesRunStatusChanged();
    }

    void Resolver::onFullIndexationFinished()
    {
        qDebug() << "full indexation finished";

        Q_EMIT fullIndexationRunStatusChanged();
    }

    void Resolver::onFileAnalysisFailed(QString path)
    {
        Q_UNUSED(path)
        // TODO
    }

    void Resolver::onFileAnalysisCompleted(QString path, FileAnalysis analysis)
    {
        const auto hashes = analysis.hashes();
        const auto allHashes = hashes.allHashes();

        _hashIdRegistrar->getOrCreateIds(allHashes)
            .handleOnEventLoop(
                this,
                [this, path, analysis, hashes, allHashes](
                                   ResultOrError<QVector<uint>, FailureType> maybeHashIds)
                {
                    if (maybeHashIds.failed())
                    {
                        qWarning() << "Resolver: failed to get IDs for hashes for path"
                                   << path;
                        return;
                    }

                    auto hashIds = maybeHashIds.result();

                    if (hashIds.size() > 1)
                        markHashesAsEquivalent(hashIds);

                    QMutexLocker lock(&_lock);

                    HashKnowledge* knowledge;
                    if (!hashes.multipleHashes())
                    {
                        knowledge = registerHash(hashes.main());
                    }
                    else
                    {
                        knowledge = nullptr;
                        for (auto const& hash : allHashes)
                        {
                            auto k = _hashToKnowledge.value(hash, nullptr);
                            if (!k)
                                continue;

                            if (!knowledge)
                                knowledge = k;
                            else
                                qDebug() << "detected multiple hashes in use for file"
                                         << path << ":" << knowledge->hash() << "and"
                                         << k->hash();
                        }

                        if (!knowledge)
                        {
                            knowledge = registerHash(hashes.main());
                        }
                    }

                    if (!knowledge)
                    {
                        qWarning() << "Resolver: failed to register hash for:" << path;
                        return;
                    }

                    knowledge->addInfo(analysis.audioData(), analysis.tagData());

                    auto fileInfo = analysis.fileInfo();

                    if (fileInfo.path().length() <= 0
                            || fileInfo.size() <= 0
                            || fileInfo.lastModifiedUtc().isNull())
                    {
                        qWarning() << "Resolver: cannot register file details for:"
                                   << path;
                        return;
                    }

                    knowledge->addPath(fileInfo.path(), fileInfo.size(),
                                       fileInfo.lastModifiedUtc(), _fullIndexationNumber);

                    knowledge->markFileAnalyzed();
                }
            );
    }

    void Resolver::onAnalyzerFinished()
    {
        if (_quickScanStatus
                          == QuickScanForNewFilesStatus::WaitingForFileAnalysisCompletion)
        {
            _quickScanStatus = QuickScanForNewFilesStatus::NotRunning;
            QTimer::singleShot(0, this, [this]() { onQuickScanForNewFilesFinished(); });
        }

        if (_fullIndexationStatus
                                == FullIndexationStatus::WaitingForFileAnalysisCompletion)
        {
            _fullIndexationStatus = FullIndexationStatus::CheckingForFileRemovals;
            QtConcurrent::run(this, &Resolver::doFullIndexationCheckForFileRemovals);
        }
    }

    Future<QString, FailureType> Resolver::findPathForHashAsync(FileHash hash)
    {
        if (hash.isNull())
        {
            qWarning() << "Resolver: cannot find path for null hash";
            return FutureError(failure);
        }

        {
            QMutexLocker lock(&_lock);

            auto it = _hashToKnowledge.find(hash);
            if (it != _hashToKnowledge.end())
            {
                auto path = it.value()->getFile();
                if (!path.isEmpty())
                {
                    return FutureResult(path);
                }
            }
        }

        auto idFuture = _hashIdRegistrar->getOrCreateId(hash);

        // TODO : check if we have it in the locations cache

        auto pathFuture =
            idFuture.thenOnAnyThreadIndirect<QString, FailureType>(
                [this, hash](FailureOr<uint> outcome) -> Future<QString, FailureType>
                {
                    if (outcome.failed())
                        return FutureError(failure);

                    auto id = outcome.result();
                    return _fileFinder->findHashAsync(id, hash);
                }
            );

        return pathFuture;
    }

    Future<QString, FailureType> Resolver::findPathForHashAsync(uint hashId)
    {
        FileHash hash;

        {
            QMutexLocker lock(&_lock);

            auto it = _idToKnowledge.find(hashId);
            if (it == _idToKnowledge.end())
            {
                qWarning() << "Resolver: hash ID" << hashId << "is unknown";
                return FutureError(failure);
            }

            hash = it.value()->hash();

            auto path = it.value()->getFile();
            if (!path.isEmpty())
            {
                return FutureResult(path);
            }
        }

        // TODO : check if we have it in the locations cache

        return _fileFinder->findHashAsync(hashId, hash);
    }

    Future<SuccessType, FailureType> Resolver::waitUntilAnyFileAnalyzed(uint hashId)
    {
        QMutexLocker lock(&_lock);

        auto it = _idToKnowledge.constFind(hashId);
        if (it == _idToKnowledge.constEnd())
        {
            qWarning() << "Resolver: hash ID" << hashId << "is unknown";
            return FutureError(failure);
        }

        return it.value()->getFutureForFirstFileAnalyzed();
    }

    void Resolver::doQuickScanForNewFilesFileSystemTraversal()
    {
        qDebug()
            << "quick scan for new files: running file system traversal (music paths)";

        auto musicPaths = this->musicPaths();

        uint fileCount = 0;
        for (QString const& musicPath : qAsConst(musicPaths))
        {
            QDirIterator it(musicPath, QDirIterator::Subdirectories); /* no symlinks */

            while (it.hasNext())
            {
                QFileInfo entry(it.next());
                if (!FileAnalyzer::isFileSupported(entry)) continue;

                auto absoluteFilePath = entry.absoluteFilePath();

                if (_fileLocations.pathHasAtLeastOneId(absoluteFilePath))
                    continue; /* not a new file */

                fileCount++;
                _analyzer->enqueueFile(absoluteFilePath);
            }
        }

        qDebug() << "quick scan for new files:"
                 << fileCount << "files added to analysis queue";

        if (_analyzer->isFinished())
        {
            _quickScanStatus = QuickScanForNewFilesStatus::NotRunning;
            QTimer::singleShot(0, this, [this]() { onQuickScanForNewFilesFinished(); });
        }
        else
        {
            _quickScanStatus =
                QuickScanForNewFilesStatus::WaitingForFileAnalysisCompletion;
        }
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
        QTimer::singleShot(0, this, [this]() { onFullIndexationFinished(); });
    }

    Resolver::HashKnowledge* Resolver::registerHash(const FileHash& hash)
    {
        if (hash.isNull())
            return nullptr; /* invalid hash */

        QMutexLocker lock(&_lock);

        auto knowledge = _hashToKnowledge.value(hash, nullptr);
        if (!knowledge)
        {
            knowledge = new HashKnowledge(this, hash, 0);
            _hashToKnowledge[hash] = knowledge;
            _hashesList.append(hash);
        }
        else if (knowledge->id() > 0)
        {
            return knowledge; /* registered already */
        }

        auto db = Database::getDatabaseForCurrentThread();
        if (!db) return knowledge; /* database not available */

        db->registerHash(hash);
        auto idOrError = db->getHashId(hash);
        if (idOrError.failed() || idOrError.result() <= 0)
            return knowledge; /* something went wrong */

        auto id = idOrError.result();
        knowledge->setId(id);
        _idToKnowledge[id] = knowledge;

        qDebug() << "got ID" << id << "for registered hash" << hash.dumpToString();

        return knowledge;
    }

    void Resolver::markHashesAsEquivalent(QVector<uint> hashes)
    {
        if (_hashRelations->areEquivalent(hashes))
            return;

        _hashRelations->markAsEquivalent(hashes);
        _historyStatistics->invalidateAllGroupStatisticsForHash(hashes[0]);

        Concurrent::runOnThreadPool<SuccessType, FailureType>(
            globalThreadPool,
            [hashes]() -> ResultOrError<SuccessType, FailureType>
            {
                auto db = Database::getDatabaseForCurrentThread();
                if (!db) return failure;

                int currentYear = QDateTime::currentDateTimeUtc().date().year();

                for (int i = 1; i < hashes.size(); ++i)
                {
                    auto result =
                            db->registerEquivalence(hashes[0], hashes[i], currentYear);

                    if (result.failed())
                        return failure;
                }

                return success;
            }
        );
    }

    QVector<QString> Resolver::getPathsThatDontMatchCurrentFullIndexationNumber()
    {
        QMutexLocker lock(&_lock);

        QVector<QString> result;

        for (auto file : qAsConst(_pathToVerifiedFile))
        {
            if (!file->hasIndexationNumber(_fullIndexationNumber))
            {
                result << file->_path;
            }
        }

        return result;
    }

    bool Resolver::haveFileForHash(const FileHash& hash)
    {
        QMutexLocker lock(&_lock);

        auto knowledge = _hashToKnowledge.value(hash, nullptr);
        return knowledge && knowledge->isAvailable();
    }

    bool Resolver::pathStillValid(const FileHash& hash, QString path)
    {
        QMutexLocker lock(&_lock);

        VerifiedFile* file = _pathToVerifiedFile.value(path, nullptr);
        if (!file) return false;

        auto knowledge = file->_parent;
        if (knowledge->hash() != hash) return false;

        return knowledge->isStillValid(file);
    }

    Nullable<FileHash> Resolver::getHashForFilePath(QString path)
    {
        QMutexLocker lock(&_lock);

        VerifiedFile* file = _pathToVerifiedFile.value(path, nullptr);
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

        VerifiedFile* file = _pathToVerifiedFile.value(path, nullptr);
        if (!file) return;

        auto knowledge = file->_parent;
        (void)knowledge->isStillValid(file);
    }

    Nullable<AudioData> Resolver::findAudioData(const FileHash& hash)
    {
        QMutexLocker lock(&_lock);

        auto knowledge = _hashToKnowledge.value(hash, nullptr);
        if (knowledge) return knowledge->audio();

        return null;
    }

    Nullable<TagData> Resolver::findTagData(const FileHash& hash)
    {
        QMutexLocker lock(&_lock);

        auto knowledge = _hashToKnowledge.value(hash, nullptr);

        if (knowledge)
        {
            const auto* bestTag = knowledge->findBestTag();

            if (bestTag)
                return *bestTag;
        }

        return null;
    }

    QVector<FileHash> Resolver::getAllHashes()
    {
        QMutexLocker lock(&_lock);
        auto copy = _hashesList.toVector();
        return copy;
    }

    QVector<CollectionTrackInfo> Resolver::getHashesTrackInfo(QVector<FileHash> hashes)
    {
        QMutexLocker lock(&_lock);

        QVector<CollectionTrackInfo> result;
        result.reserve(hashes.size());

        for (auto& hash : qAsConst(hashes))
        {
            auto knowledge = _hashToKnowledge.value(hash, nullptr);
            if (!knowledge) continue;

            auto lengthInMilliseconds = knowledge->audio().trackLengthMilliseconds();
            if (lengthInMilliseconds > std::numeric_limits<qint32>::max())
            {
                lengthInMilliseconds = 0;
            }

            CollectionTrackInfo info(hash, knowledge->isAvailable(),
                                     knowledge->quickTitle(), knowledge->quickArtist(),
                                     knowledge->quickAlbum(),
                                     knowledge->quickAlbumArtist(),
                                     (qint32)lengthInMilliseconds);

            result.append(info);
        }

        return result;
    }

    CollectionTrackInfo Resolver::getHashTrackInfo(uint hashId)
    {
        QMutexLocker lock(&_lock);

        auto knowledge = _idToKnowledge.value(hashId, nullptr);
        if (!knowledge) return {};

        auto lengthInMilliseconds = knowledge->audio().trackLengthMilliseconds();
        if (lengthInMilliseconds > std::numeric_limits<qint32>::max())
        {
            lengthInMilliseconds = 0;
        }

        CollectionTrackInfo info(knowledge->hash(), knowledge->isAvailable(),
                                 knowledge->quickTitle(), knowledge->quickArtist(),
                                 knowledge->quickAlbum(), knowledge->quickAlbumArtist(),
                                 qint32(lengthInMilliseconds));

        return info;
    }

    FileHash Resolver::getHashByID(uint id)
    {
        QMutexLocker lock(&_lock);

        auto knowledge = _idToKnowledge.value(id, nullptr);
        if (knowledge) return knowledge->hash();

        qWarning() << "Resolver::getHashByID: ID" << id << "not found";

        return FileHash(); /* empty (invalid) hash */
    }

    uint Resolver::getID(const FileHash& hash)
    {
        QMutexLocker lock(&_lock);

        auto knowledge = _hashToKnowledge.value(hash, nullptr);
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
            auto knowledge = _hashToKnowledge.value(hash, nullptr);
            if (!knowledge) continue;

            result.append(QPair<uint, FileHash>(knowledge->id(), hash));
        }

        return result;
    }
}
