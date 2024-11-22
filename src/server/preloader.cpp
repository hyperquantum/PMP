/*
    Copyright (C) 2016-2024, Kevin Andre <hyperquantum@gmail.com>

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

#include "preloader.h"

#include "common/concurrent.h"
#include "common/fileanalyzer.h"

#include "playerqueue.h"
#include "queueentry.h"
#include "resolver.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutexLocker>
#include <QSaveFile>
#include <QStandardPaths>
#include <QtDebug>
#include <QTemporaryFile>
#include <QThreadPool>
#include <QTimer>

namespace PMP::Server
{
    /* ======================== PreloadedFileLock ======================= */

    PreloadedFile::PreloadedFile()
    {
        //
    }

    PreloadedFile::PreloadedFile(Preloader* preloader,
                                 std::function<void (Preloader*)> cleaner,
                                 QString filename)
     : AbstractHandle<QObjectResourceKeeper<Preloader> >(
        std::make_shared<QObjectResourceKeeper<Preloader> >(preloader, cleaner)
       ),
       _filename(filename)
    {
        //
    }

    /* ========================== PreloadTrack ========================== */

    class Preloader::PreloadTrack
    {
    public:
        PreloadTrack(FileHash hash, QString filename)
         : _status(Initial), _hash(hash), _filename(filename)
        {
            //
        }

        ~PreloadTrack()
        {
            cleanup();
        }

        enum Status { Initial = 0, Processing, Preloaded, Failed, CleanedUp };

        Status status() const;
        FileHash const& hash() const;
        QString originalFilename() const;

        void setToLoading();
        void setToFailed();
        void setToLoaded(QString cacheFile);
        bool cleanup();

        QString getCachedFile() const;

    private:
        Status _status;
        FileHash _hash;
        QString _filename;
        QString _cacheFile;
    };

    Preloader::PreloadTrack::Status Preloader::PreloadTrack::status() const
    {
        return _status;
    }

    FileHash const& Preloader::PreloadTrack::hash() const
    {
        return _hash;
    }

    QString Preloader::PreloadTrack::originalFilename() const
    {
        return _filename;
    }

    void Preloader::PreloadTrack::setToLoading()
    {
        _status = Status::Processing;
    }

    void Preloader::PreloadTrack::setToFailed()
    {
        _status = Status::Failed;
    }

    void Preloader::PreloadTrack::setToLoaded(QString cacheFile)
    {
        _cacheFile = cacheFile;
        _status = Status::Preloaded;
    }

    bool Preloader::PreloadTrack::cleanup()
    {
        if (_status == Status::Processing) return false; /* a file will appear */
        if (_status != Status::Preloaded || _cacheFile.isEmpty()) return true;
        if (!QFile::remove(_cacheFile)) return false;

        _cacheFile.clear();
        _status = Status::CleanedUp;
        return true;
    }

    QString Preloader::PreloadTrack::getCachedFile() const
    {
        return _cacheFile;
    }

    /* ========================== Preloader ========================== */

    Preloader::Preloader(QObject* parent, PlayerQueue* queue, Resolver* resolver)
     : QObject(parent),
       _queue(queue), _resolver(resolver),
       _jobsRunning(0),
       _firstTrackCheckTimerRunning(false),
       _preloadCheckTimerRunning(false),
       _cacheExpirationCheckTimerRunning(false)
    {
        connect(queue, &PlayerQueue::entryAdded, this, &Preloader::queueEntryAdded);
        connect(queue, &PlayerQueue::entryRemoved, this, &Preloader::queueEntryRemoved);
        connect(queue, &PlayerQueue::entryMoved, this, &Preloader::queueEntryMoved);
        connect(
            queue, &PlayerQueue::firstTrackChanged,
            this, &Preloader::firstTrackInQueueChanged
        );

        scheduleCheckForTracksToPreload();
    }

    Preloader::~Preloader()
    {
        /* delete cache files */
        for (auto* track : _tracksByQueueID.values())
        {
            if (track->status() != PreloadTrack::Status::Preloaded) continue;

            QFile::remove(track->getCachedFile());
        }
    }

    bool Preloader::havePreloadedFileQuickCheck(uint queueId)
    {
        auto track = _tracksByQueueID.value(queueId, nullptr);
        if (!track) return false;

        return track->status() == PreloadTrack::Preloaded;
    }

    PreloadedFile Preloader::getPreloadedCacheFile(uint queueID)
    {
        auto track = _tracksByQueueID.value(queueID, nullptr);
        if (!track) return PreloadedFile();

        auto filename = track->getCachedFile();
        if (filename.isEmpty()) return PreloadedFile();

        if (!QFileInfo::exists(filename))
        {
            /* the file that was preloaded has disappeared */
            _tracksByQueueID.remove(queueID);
            delete track;
            return PreloadedFile();
        }

        doLock(queueID);

        return PreloadedFile(
            this,
            [queueID](Preloader* preloader)
            {
                if (preloader) preloader->doUnlock(queueID);
            },
            filename
        );
    }

    void Preloader::cleanupOldFiles()
    {
        QString tempDir = QDir::temp().absolutePath() + "/PMP-preload-cache";

        QDir dir(tempDir);
        if (!dir.exists()) { return; }

        auto threshhold = QDate::currentDate().addDays(-10).startOfDay();

        const auto files =
            dir.entryInfoList(
                QDir::Files | QDir::NoSymLinks | QDir::Readable | QDir::Writable
            );

        for (auto& file : files)
        {
            if (file.lastModified() >= threshhold) continue;
            if (!FileAnalyzer::isExtensionSupported(file.suffix())) continue;

            qDebug() << "deleting old file from preload-cache:" << file.fileName();
            QFile::remove(file.absoluteFilePath());
        }
    }

    void Preloader::queueEntryAdded(qint32 offset, quint32 queueID)
    {
        Q_UNUSED(queueID);

        if (offset >= PRELOAD_RANGE) return;

        scheduleCheckForTracksToPreload();
    }

    void Preloader::queueEntryRemoved(qint32 offset, quint32 queueID)
    {
        if (offset < PRELOAD_RANGE)
        {
            scheduleCheckForTracksToPreload();
        }

        _tracksRemoved.append(queueID);
        scheduleCheckForCacheEntriesToDelete();
    }

    void Preloader::queueEntryMoved(qint32 fromOffset, qint32 toOffset, quint32 queueID)
    {
        Q_UNUSED(queueID);

        if (toOffset >= PRELOAD_RANGE && fromOffset >= PRELOAD_RANGE)
            return;

        scheduleCheckForTracksToPreload();
    }

    void Preloader::firstTrackInQueueChanged(int index, uint queueId)
    {
        Q_UNUSED(queueId)

        if (index >= 0)
            scheduleFirstTrackCheck();
    }

    void Preloader::scheduleFirstTrackCheck()
    {
        if (_firstTrackCheckTimerRunning) return;

        _firstTrackCheckTimerRunning = true;
        qDebug() << "first track check triggered";
        QTimer::singleShot(25, this, &Preloader::checkFirstTrack);
    }

    void Preloader::scheduleCheckForTracksToPreload()
    {
        if (_preloadCheckTimerRunning) return;

        _preloadCheckTimerRunning = true;
        qDebug() << "preload check triggered";
        QTimer::singleShot(250, this, &Preloader::checkForTracksToPreload);
    }

    void Preloader::checkFirstTrack()
    {
        _firstTrackCheckTimerRunning = false;
        qDebug() << "checking if first track needs preloading";

        auto firstTrack = _queue->peekFirstTrackEntry();
        if (!firstTrack) return;

        checkToPreloadTrack(firstTrack);
        checkForJobsToStart();
    }

    void Preloader::checkForTracksToPreload()
    {
        _preloadCheckTimerRunning = false;
        qDebug() << "running preload check";

        auto queueEntries = _queue->entries(0, PRELOAD_RANGE);
        for (auto& entry : queueEntries)
        {
            checkToPreloadTrack(entry);
        }

        checkForJobsToStart();
    }

    void Preloader::checkToPreloadTrack(QSharedPointer<QueueEntry> entry)
    {
        if (!entry->isTrack())
            return;

        FileHash hash = entry->hash().value();
        Nullable<QString> filename = entry->filename();

        auto id = entry->queueID();
        auto track = _tracksByQueueID.value(id, nullptr);

        if (track && track->status() == PreloadTrack::Preloaded)
        {
            if (QFileInfo::exists(track->getCachedFile()))
                return; /* preloaded file is present */

            /* file has gone missing (it was in a TEMP folder after all) */
            qDebug() << "cached file has gone missing for queue ID" << id;
            _tracksByQueueID.remove(id);
            delete track;
            track = nullptr;
        }

        if (track) return;

        qDebug() << "putting queue ID" << id << "on the list for preloading";

        track = new PreloadTrack(hash, filename.valueOr({}));

        _tracksByQueueID.insert(id, track);
        _tracksToPreload.append(id);
    }

    Future<QString, FailureType> Preloader::preloadAsync(uint queueId, FileHash hash,
                                                         QString originalFilename)
    {
        if (!originalFilename.isEmpty()
                && _resolver->pathStillValid(hash, originalFilename))
        {
            return Concurrent::runOnThreadPool<QString, FailureType>(
                globalThreadPool,
                [queueId, originalFilename]()
                {
                    return runPreload(queueId, originalFilename);
                }
            );
        }

        qDebug() << "Preloader: don't have a filename yet for queue ID" << queueId
                 << "which has hash" << hash;

        return
            _resolver->findPathForHashAsync(hash)
                .thenOnThreadPool<QString, FailureType>(
                    globalThreadPool,
                    [queueId](FailureOr<QString> outcome) -> FailureOr<QString>
                    {
                        if (outcome.failed())
                            return failure;

                        auto path = outcome.result();

                        qDebug() << "Preloader: found path" << path
                                 << "for queue ID" << queueId;

                        return runPreload(queueId, path);
                    }
                );
    }

    ResultOrError<QString, FailureType> Preloader::runPreload(uint queueId,
                                                              QString originalFilename)
    {
        qDebug() << "Preloader: will process" << originalFilename
                 << "for queue ID" << queueId;

        QFileInfo fileInfo(originalFilename);
        if (!fileInfo.isFile() || !fileInfo.isReadable())
        {
            qWarning() << "Preloader: not a file or not readable:" << originalFilename;
            return failure;
        }

        QString extension = fileInfo.suffix();

        QFile file(originalFilename);
        if (!file.open(QIODevice::ReadOnly))
        {
            qWarning() << "Preloader: failed to open file:" << originalFilename;
            return failure;
        }

        QByteArray contents = file.readAll();
        file.close();

        qDebug() << "Preloader: read" << contents.size() << "bytes from"
                 << originalFilename << "for queue ID" << queueId;

        if (!FileAnalyzer::preprocessFileForPlayback(contents, extension))
        {
            qWarning() << "Preloader: failed to preprocess file" << originalFilename;
            return failure;
        }

        QString tempDir;
        if (QDir::temp().mkpath("PMP-preload-cache"))
            tempDir = QDir::temp().absolutePath() + "/PMP-preload-cache";
        else
            tempDir = QDir::temp().absolutePath();

        QString saveName = tempDir + "/" + tempFilename(queueId, extension);
        QFileInfo saveFileInfo(saveName);
        if (saveFileInfo.exists())
        {
            qWarning() << "Preloader: name collision for temp file" << saveName;
            return failure;
        }

        QSaveFile saveFile(saveName);
        if (!saveFile.open(QIODevice::WriteOnly))
        {
            qWarning() << "Preloader: failed to open temp file for writing:" << saveName;
            return failure;
        }

        saveFile.write(contents);
        if (!saveFile.commit())
        {
            QFile::remove(saveName);
            qWarning() << "Preloader: failed to commit changes to temp file" << saveName;
            return failure;
        }

        /* success */
        qDebug() << "Preloader: successfully preloaded file for queue ID" << queueId
                 << "into temp file:" << saveName;
        return saveName;
    }

    QString Preloader::tempFilename(uint queueId, QString extension)
    {
        return "P" + QString::number(QCoreApplication::applicationPid())
                + "-Q" + QString::number(queueId) + "." + extension;
    }

    void Preloader::checkForJobsToStart()
    {
        uint iterations = 0;
        while (_jobsRunning < 2 && !_tracksToPreload.empty())
        {
             /* iteration limit to prevent blocking for too long */
            if (iterations >= 5) break;
            iterations++;

            auto queueId = _tracksToPreload.first();
            _tracksToPreload.removeFirst();

            auto track = _tracksByQueueID.value(queueId, nullptr);
            if (!track) continue; /* already removed */
            if (track->status() != PreloadTrack::Status::Initial) continue;

            qDebug() << "starting track preload task for QID" << queueId;

            track->setToLoading();

            auto future = preloadAsync(queueId, track->hash(), track->originalFilename());
            _jobsRunning++;

            future.handleOnEventLoop(
                this,
                [this, queueId](FailureOr<QString> outcome)
                {
                    if (outcome.succeeded())
                        preloadFinished(queueId, outcome.result());
                    else
                        preloadFailed(queueId);
                }
            );
        }
    }

    void Preloader::scheduleCheckForCacheEntriesToDelete()
    {
        if (_cacheExpirationCheckTimerRunning) return;

        _cacheExpirationCheckTimerRunning = true;

        qDebug() << "preload-cache expiration check triggered";
        QTimer::singleShot(500, this, &Preloader::checkForCacheExpiration);
    }

    void Preloader::checkForCacheExpiration()
    {
        _cacheExpirationCheckTimerRunning = false;

        int max = 3;
        for (int i = 0; i < max && i < _tracksRemoved.size(); ++i)
        {
            auto id = _tracksRemoved[i];

            if (_lockedQueueIds.contains(id)) continue;

            auto track = _tracksByQueueID.value(id, nullptr);
            if (track && track->status() == PreloadTrack::Status::Processing)
                continue; /* we'll have to wait */

            qDebug() << "deleting preload-cache info (if any) for QID" << id;

            max--;
            _tracksRemoved.removeAt(i);
            i--;

            if (!track)
            {
                /* no object to delete */
            }
            else if (track->cleanup())
            {
                _tracksByQueueID.remove(id);
                delete track;
            }
            else
            {
                /* we will try again later */
                _tracksRemoved.append(id);
            }
        }
    }

    void Preloader::preloadFailed(uint queueID)
    {
        qDebug() << "Preloader: preload job FAILED for QID" << queueID;

        _jobsRunning--;

        auto track = _tracksByQueueID.value(queueID, nullptr);
        if (track) track->setToFailed();

        checkForJobsToStart();
    }

    void Preloader::doLock(uint queueId)
    {
        _lockedQueueIds[queueId]++;
    }

    void Preloader::doUnlock(uint queueId)
    {
        auto lockIterator = _lockedQueueIds.find(queueId);
        if (lockIterator == _lockedQueueIds.end())
        {
            qWarning() << "Preloader::doUnlock: no lock found for QID" << queueId << "!";
            return;
        }

        auto& lockCount = lockIterator.value();
        if (lockCount > 0)
        {
            lockCount--;
        }
        else
        {
            qWarning() << "Preloader::doUnlock: lock count for QID" << queueId
                       << "already zero!";
        }

        if (lockCount > 0) return; /* not completely unlocked yet */

        _lockedQueueIds.erase(lockIterator);
        scheduleCheckForCacheEntriesToDelete();
    }

    void Preloader::preloadFinished(uint queueID, QString cacheFile)
    {
        qDebug() << "Preloader: preload job finished for QID" << queueID
                 << ": saved as" << cacheFile;

        _jobsRunning--;

        auto track = _tracksByQueueID.value(queueID, nullptr);
        if (track)
        {
            track->setToLoaded(cacheFile);
        }
        else
        {
            qDebug() << "QID" << queueID
                     << "seems to be no longer needed, discarding cache file";
            QFile::remove(cacheFile);
        }

        checkForJobsToStart();
        scheduleCheckForCacheEntriesToDelete();

        Q_EMIT trackPreloaded(queueID);
    }

}
