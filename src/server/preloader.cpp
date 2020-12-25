/*
    Copyright (C) 2016-2020, Kevin Andre <hyperquantum@gmail.com>

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

namespace PMP {

    /* ========================== TrackPreloadTask ========================== */

    TrackPreloadTask::TrackPreloadTask(Resolver* resolver,
                                       uint queueID, FileHash hash, QString filename)
     : _resolver(resolver),
       _queueID(queueID), _hash(hash), _originalFilename(filename)
    {
        //
    }

    void TrackPreloadTask::run() {
        QString filename = _originalFilename;

        if (!_hash.isNull()) {
            if (filename.isEmpty() || !_resolver->pathStillValid(_hash, filename))
            {
                filename = _resolver->findPath(_hash, false);
            }
        }

        if (filename.isEmpty()) {
            Q_EMIT preloadFailed(_queueID, 123);
            return;
        }

        QFileInfo fileInfo(filename);
        if (!fileInfo.isFile() || !fileInfo.isReadable()) {
            Q_EMIT preloadFailed(_queueID, 234);
            return;
        }

        QString extension = fileInfo.suffix();

        QFile file(filename);
        if (!file.open(QIODevice::ReadOnly)) {
            Q_EMIT preloadFailed(_queueID, 456);
            return;
        }

        QByteArray contents = file.readAll();
        file.close();

        qDebug() << "TrackPreloadTask: read" << contents.size() << "bytes from"
                 << filename;

        if (!FileAnalyzer::preprocessFileForPlayback(contents, extension)) {
            Q_EMIT preloadFailed(_queueID, 678);
            return;
        }

        QString tempDir;
        if (QDir::temp().mkpath("PMP-preload-cache")) {
            tempDir = QDir::temp().absolutePath() + "/PMP-preload-cache";
        }
        else {
            tempDir = QDir::temp().absolutePath();
        }

        QString saveName = tempDir + "/" + tempFilename(_queueID, extension);
        QFileInfo saveFileInfo(saveName);
        if (saveFileInfo.exists()) {
            /* name collision, give up */
            Q_EMIT preloadFailed(_queueID, 789);
            return;
        }

        QSaveFile saveFile(saveName);
        if (!saveFile.open(QIODevice::WriteOnly)){
            Q_EMIT preloadFailed(_queueID, 987);
            return;
        }

        saveFile.write(contents);
        if (!saveFile.commit()) {
            QFile::remove(saveName);
            Q_EMIT preloadFailed(_queueID, 876);
            return;
        }

        /* success */
        Q_EMIT preloadFinished(_queueID, saveName);
    }

    QString TrackPreloadTask::tempFilename(uint queueID, QString extension) {
        return "P" + QString::number(QCoreApplication::applicationPid())
                + "-Q" + QString::number(queueID) + "." + extension;
    }

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

    class Preloader::PreloadTrack {
    public:
        PreloadTrack(FileHash hash, QString filename)
         : _status(Initial), _hash(hash), _filename(filename)
        {
            //
        }

        ~PreloadTrack() {
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

    Preloader::PreloadTrack::Status Preloader::PreloadTrack::status() const {
        return _status;
    }

    FileHash const& Preloader::PreloadTrack::hash() const {
        return _hash;
    }

    QString Preloader::PreloadTrack::originalFilename() const {
        return _filename;
    }

    void Preloader::PreloadTrack::setToLoading() {
        _status = Status::Processing;
    }

    void Preloader::PreloadTrack::setToFailed() {
        _status = Status::Failed;
    }

    void Preloader::PreloadTrack::setToLoaded(QString cacheFile) {
        _cacheFile = cacheFile;
        _status = Status::Preloaded;
    }

    bool Preloader::PreloadTrack::cleanup() {
        if (_status == Status::Processing) return false; /* a file will appear */
        if (_status != Status::Preloaded || _cacheFile.isEmpty()) return true;
        if (!QFile::remove(_cacheFile)) return false;

        _cacheFile.clear();
        _status = Status::CleanedUp;
        return true;
    }

    QString Preloader::PreloadTrack::getCachedFile() const {
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

    Preloader::~Preloader() {
        /* delete cache files */
        Q_FOREACH(PreloadTrack* track, _tracksByQueueID.values()) {
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

    PreloadedFile Preloader::getPreloadedCacheFile(uint queueID) {
        auto track = _tracksByQueueID.value(queueID, nullptr);
        if (!track) return PreloadedFile();

        auto filename = track->getCachedFile();
        if (filename.isEmpty()) return PreloadedFile();

        if (!QFileInfo::exists(filename)) {
            /* the file that was preloaded has disappeared */
            _tracksByQueueID.remove(queueID);
            delete track;
            return PreloadedFile();
        }

        doLock(queueID);

        return PreloadedFile(
            this,
            [queueID](Preloader* preloader) {
                if (preloader) preloader->doUnlock(queueID);
            },
            filename
        );
    }

    void Preloader::cleanupOldFiles() {
        QString tempDir = QDir::temp().absolutePath() + "/PMP-preload-cache";

        QDir dir(tempDir);
        if (!dir.exists()) { return; }

        auto threshhold = QDateTime(QDate::currentDate().addDays(-10));

        auto files =
            dir.entryInfoList(
                QDir::Files | QDir::NoSymLinks | QDir::Readable | QDir::Writable
            );

        Q_FOREACH(auto file, files) {
            if (file.lastModified() >= threshhold) continue;
            if (!FileAnalyzer::isExtensionSupported(file.suffix())) continue;

            qDebug() << "deleting old file from preload-cache:" << file.fileName();
            QFile::remove(file.absoluteFilePath());
        }
    }

    void Preloader::queueEntryAdded(quint32 offset, quint32 queueID) {
        (void)queueID;

        if (offset >= PRELOAD_RANGE) return;

        scheduleCheckForTracksToPreload();
    }

    void Preloader::queueEntryRemoved(quint32 offset, quint32 queueID) {
        if (offset < PRELOAD_RANGE) {
            scheduleCheckForTracksToPreload();
        }

        _tracksRemoved.append(queueID);
        scheduleCheckForCacheEntriesToDelete();
    }

    void Preloader::queueEntryMoved(quint32 fromOffset, quint32 toOffset, quint32 queueID)
    {
        (void)queueID;

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

    void Preloader::scheduleCheckForTracksToPreload() {
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

    void Preloader::checkForTracksToPreload() {
        _preloadCheckTimerRunning = false;
        qDebug() << "running preload check";

        QList<QueueEntry*> queueEntries = _queue->entries(0, PRELOAD_RANGE);
        Q_FOREACH(QueueEntry* entry, queueEntries) {
            checkToPreloadTrack(entry);
        }

        checkForJobsToStart();
    }

    void Preloader::checkToPreloadTrack(QueueEntry* entry)
    {
        const FileHash* hash = entry->hash();
        const QString* filename = entry->filename();
        if (!hash && !filename) return;

        auto id = entry->queueID();
        auto track = _tracksByQueueID.value(id, nullptr);

        if (track && track->status() == PreloadTrack::Preloaded) {
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

        track =
            new PreloadTrack(hash ? *hash : FileHash(), filename ? *filename : "");

        _tracksByQueueID.insert(id, track);
        _tracksToPreload.append(id);
    }

    void Preloader::checkForJobsToStart() {
        uint iterations = 0;
        while (_jobsRunning < 2 && !_tracksToPreload.empty()) {
             /* iteration limit to prevent blocking for too long */
            if (iterations >= 5) break;
            iterations++;

            auto id = _tracksToPreload.first();
            _tracksToPreload.removeFirst();

            auto track = _tracksByQueueID.value(id, nullptr);
            if (!track) continue; /* already removed */
            if (track->status() != PreloadTrack::Status::Initial) continue;

            qDebug() << "starting track preload task for QID" << id;

            track->setToLoading();

            auto task =
                new TrackPreloadTask(
                    _resolver, id, track->hash(), track->originalFilename()
                );

            connect(
                task, &TrackPreloadTask::preloadFailed, this, &Preloader::preloadFailed
            );
            connect(
                task, &TrackPreloadTask::preloadFinished,
                this, &Preloader::preloadFinished
            );

            QThreadPool::globalInstance()->start(task);
            _jobsRunning++;
        }
    }

    void Preloader::scheduleCheckForCacheEntriesToDelete() {
        if (_cacheExpirationCheckTimerRunning) return;

        _cacheExpirationCheckTimerRunning = true;

        qDebug() << "preload-cache expiration check triggered";
        QTimer::singleShot(500, this, &Preloader::checkForCacheExpiration);
    }

    void Preloader::checkForCacheExpiration() {
        _cacheExpirationCheckTimerRunning = false;

        int max = 3;
        for (int i = 0; i < max && i < _tracksRemoved.size(); ++i) {
            auto id = _tracksRemoved[i];

            if (_lockedQueueIds.contains(id)) continue;

            auto track = _tracksByQueueID.value(id, nullptr);
            if (track && track->status() == PreloadTrack::Status::Processing)
                continue; /* we'll have to wait */

            qDebug() << "deleting preload-cache info (if any) for QID" << id;

            max--;
            _tracksRemoved.removeAt(i);
            i--;

            if (!track) {
                /* no object to delete */
            }
            else if (track->cleanup()) {
                _tracksByQueueID.remove(id);
                delete track;
            }
            else {
                /* we will try again later */
                _tracksRemoved.append(id);
            }
        }
    }

    void Preloader::preloadFailed(uint queueID, int reason) {
        qDebug() << "preload job FAILED for QID" << queueID << "with reason" << reason;

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
        if (lockIterator == _lockedQueueIds.end()) {
            qWarning() << "Preloader::doUnlock: no lock found for QID" << queueId << "!";
            return;
        }

        auto& lockCount = lockIterator.value();
        if (lockCount > 0) {
            lockCount--;
        }
        else {
            qWarning() << "Preloader::doUnlock: lock count for QID" << queueId
                       << "already zero!";
        }

        if (lockCount > 0) return; /* not completely unlocked yet */

        _lockedQueueIds.erase(lockIterator);
        scheduleCheckForCacheEntriesToDelete();
    }

    void Preloader::preloadFinished(uint queueID, QString cacheFile) {
        qDebug() << "preload job finished for QID" << queueID
                 << ": saved as" << cacheFile;

        _jobsRunning--;

        auto track = _tracksByQueueID.value(queueID, nullptr);
        if (track) {
            track->setToLoaded(cacheFile);
        }
        else {
            qDebug() << "QID" << queueID
                     << "seems to be no longer needed, discarding cache file";
            QFile::remove(cacheFile);
        }

        checkForJobsToStart();
        scheduleCheckForCacheEntriesToDelete();

        Q_EMIT trackPreloaded(queueID);
    }

}
