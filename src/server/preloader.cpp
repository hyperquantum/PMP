/*
    Copyright (C) 2016, Kevin Andre <hyperquantum@gmail.com>

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

#include "queue.h"
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

        if (!_hash.empty()) {
            if (filename.isEmpty() || !_resolver->pathStillValid(_hash, filename))
            {
                filename = _resolver->findPath(_hash, false);
            }
        }

        if (filename.isEmpty()) {
            emit preloadFailed(_queueID, 123);
            return;
        }

        QFileInfo fileInfo(filename);
        if (!fileInfo.isFile() || !fileInfo.isReadable()) {
            emit preloadFailed(_queueID, 234);
            return;
        }

        QString extension = fileInfo.suffix();

        QFile file(filename);
        if (!file.open(QIODevice::ReadOnly)) {
            emit preloadFailed(_queueID, 456);
            return;
        }

        QByteArray contents = file.readAll();
        file.close();

        qDebug() << "TrackPreloadTask: read" << contents.size() << "bytes from"
                 << filename;

        if (!FileAnalyzer::preprocessFileForPlayback(contents, extension)) {
            emit preloadFailed(_queueID, 678);
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
            emit preloadFailed(_queueID, 789);
            return;
        }

        QSaveFile saveFile(saveName);
        if (!saveFile.open(QIODevice::WriteOnly)){
            emit preloadFailed(_queueID, 987);
            return;
        }

        saveFile.write(contents);
        if (!saveFile.commit()) {
            QFile::remove(saveName);
            emit preloadFailed(_queueID, 876);
            return;
        }

        /* success */
        emit preloadFinished(_queueID, saveName);
    }

    QString TrackPreloadTask::tempFilename(uint queueID, QString extension) {
        return "P" + QString::number(QCoreApplication::applicationPid())
                + "-Q" + QString::number(queueID) + "." + extension;
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

    Preloader::Preloader(QObject* parent, Queue* queue, Resolver* resolver)
     : QObject(parent),
       _doNotDeleteQueueID(0), _jobsRunning(0),
       _preloadCheckTimerRunning(false), _cacheExpirationCheckTimerRunning(false),
       _queue(queue), _resolver(resolver)
    {
        connect(queue, &Queue::entryAdded, this, &Preloader::queueEntryAdded);
        connect(queue, &Queue::entryRemoved, this, &Preloader::queueEntryRemoved);
        connect(queue, &Queue::entryMoved, this, &Preloader::queueEntryMoved);

        scheduleCheckForTracksToPreload();
    }

    Preloader::~Preloader() {
        /* delete cache files */
        Q_FOREACH(PreloadTrack* track, _tracksByQueueID.values()) {
            if (track->status() != PreloadTrack::Status::Preloaded) continue;

            QFile::remove(track->getCachedFile());
        }
    }

    QString Preloader::getPreloadedCacheFileAndLock(uint queueID) {
        _doNotDeleteQueueID = queueID; /* lock : prevent file getting cleaned up */

        auto track = _tracksByQueueID.value(queueID, nullptr);
        if (!track) return "";

        return track->getCachedFile();
    }

    void Preloader::lockNone() {
        _doNotDeleteQueueID = 0;

        scheduleCheckForCacheEntriesToDelete();
    }

    void Preloader::cleanupOldFiles() {
        QString tempDir = QDir::temp().absolutePath() + "/PMP-preload-cache";

        QDir dir(tempDir);
        if (!dir.exists()) { return; }

        QDateTime threshhold = QDateTime(QDate::currentDate().addDays(-20));

        auto files =
            dir.entryInfoList(
                QDir::Files | QDir::NoSymLinks | QDir::Readable | QDir::Writable
            );

        Q_FOREACH(auto file, files) {
            if (file.lastModified() >= threshhold) continue;
            if (!FileAnalyzer::isExtensionSupported(file.suffix())) continue;

            qDebug() << "Preloader: deleting old file:" << file.fileName();
            QFile::remove(file.absoluteFilePath());
        }
    }

    void Preloader::queueEntryAdded(quint32 offset, quint32 queueID) {
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
        if (toOffset >= PRELOAD_RANGE && fromOffset >= PRELOAD_RANGE)
            return;

        scheduleCheckForTracksToPreload();
    }

    void Preloader::scheduleCheckForTracksToPreload() {
        if (_preloadCheckTimerRunning) return;

        _preloadCheckTimerRunning = true;
        qDebug() << "Preloader: preload check triggered";
        QTimer::singleShot(250, this, SLOT(checkForTracksToPreload()));
    }

    void Preloader::checkForTracksToPreload() {
        _preloadCheckTimerRunning = false;
        qDebug() << "Preloader: running preload check";

        QList<QueueEntry*> queueEntries = _queue->entries(0, PRELOAD_RANGE);
        Q_FOREACH(QueueEntry* entry, queueEntries) {
            const FileHash* hash = entry->hash();
            const QString* filename = entry->filename();
            if (!hash && !filename) continue;

            auto id = entry->queueID();
            auto track = _tracksByQueueID.value(id, nullptr);

            if (!track) {
                qDebug() << "Preloader: putting QID" << id
                         << "on the list for preloading";

                track =
                    new PreloadTrack(
                        hash ? *hash : FileHash(), filename ? *filename : ""
                    );

                _tracksByQueueID.insert(id, track);
                _tracksToPreload.append(id);
            }
        }

        checkForJobsToStart();
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

            qDebug() << "Preloader: starting load task for QID" << id;

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

        qDebug() << "Preloader: cache expiration check triggered";
        QTimer::singleShot(250, this, SLOT(checkForCacheExpiration()));
    }

    void Preloader::checkForCacheExpiration() {
        _cacheExpirationCheckTimerRunning = false;

        int max = 3;
        for (int i = 0; i < max && i < _tracksRemoved.size(); ++i) {
            auto id = _tracksRemoved[i];

            if (id == _doNotDeleteQueueID) continue;

            auto track = _tracksByQueueID.value(id, nullptr);
            if (track && track->status() == PreloadTrack::Status::Processing) {
                continue; /* we'll have to wait */
            }

            qDebug() << "Preloader: deleting cache info (if any) for QID" << id;

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
        qDebug() << "Preloader: job FAILED for QID" << queueID << "with reason" << reason;

        _jobsRunning--;

        auto track = _tracksByQueueID.value(queueID, nullptr);
        if (track) track->setToFailed();

        checkForJobsToStart();
    }

    void Preloader::preloadFinished(uint queueID, QString cacheFile) {
        qDebug() << "Preloader: job finished for QID" << queueID;
        if (queueID % 10 == 1)
            qDebug() << "Preloader:" << queueID << "saved as" << cacheFile;

        _jobsRunning--;

        auto track = _tracksByQueueID.value(queueID, nullptr);
        if (track) {
            track->setToLoaded(cacheFile);
        }
        else {
            qDebug() << "Preloader: apparently"
                     << queueID << "is no longer needed, discarding";
            QFile::remove(cacheFile);
        }

        checkForJobsToStart();
        scheduleCheckForCacheEntriesToDelete();
    }

}