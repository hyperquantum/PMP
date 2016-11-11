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

#ifndef PMP_PRELOADER_H
#define PMP_PRELOADER_H

#include "common/filehash.h"

#include <QHash>
#include <QMutex>
#include <QList>
#include <QObject>
#include <QRunnable>
//#include <QTemporaryFile>

namespace PMP {

    class Queue;
    class Resolver;

    class TrackPreloadTask : public QObject, public QRunnable {
        Q_OBJECT
    public:
        TrackPreloadTask(Resolver* resolver,
                         uint queueID, FileHash hash, QString filename);

        void run();

    Q_SIGNALS:
        void preloadFinished(uint queueID, QString cacheFile);
        void preloadFailed(uint queueID, int reason);

    private:
        QString tempFilename(uint queueID, QString extension);

        Resolver* _resolver;
        uint _queueID;
        FileHash _hash;
        QString _originalFilename;
    };

    class Preloader : public QObject {
        Q_OBJECT
    public:
        Preloader(QObject* parent, Queue* queue, Resolver* resolver);

        ~Preloader();

        QString getPreloadedCacheFileAndLock(uint queueID);
        void lockNone();

        static void cleanupOldFiles();

    private slots:
        void queueEntryAdded(quint32 offset, quint32 queueID);
        void queueEntryRemoved(quint32 offset, quint32 queueID);
        void queueEntryMoved(quint32 fromOffset, quint32 toOffset, quint32 queueID);

        void scheduleCheckForTracksToPreload();
        void checkForTracksToPreload();
        void scheduleCheckForCacheEntriesToDelete();
        void checkForCacheExpiration();

        void checkForJobsToStart();

        void preloadFinished(uint queueID, QString cacheFile);
        void preloadFailed(uint queueID, int reason);

    private:
        static const int PRELOAD_RANGE = 5;

        class PreloadTrack;

        uint _doNotDeleteQueueID;
        uint _jobsRunning;
        bool _preloadCheckTimerRunning;
        bool _cacheExpirationCheckTimerRunning;
        Queue* _queue;
        Resolver* _resolver;
        QHash<uint, PreloadTrack*> _tracksByQueueID;
        QList<uint> _tracksToPreload;
        QList<uint> _tracksRemoved;
    };
}
#endif
