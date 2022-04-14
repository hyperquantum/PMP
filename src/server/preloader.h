/*
    Copyright (C) 2016-2022, Kevin Andre <hyperquantum@gmail.com>

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

#include "common/abstracthandle.h"
#include "common/filehash.h"
#include "common/future.h"
#include "common/qobjectresourcekeeper.h"

#include <QHash>
#include <QMutex>
#include <QList>
#include <QObject>

namespace PMP
{
    class PlayerQueue;
    class Resolver;

    class Preloader;

    class PreloadedFile
        : private AbstractHandle<QObjectResourceKeeper<Preloader>>
    {
    public:
        PreloadedFile();
        PreloadedFile(Preloader* preloader, std::function<void (Preloader*)> cleaner,
                      QString filename);

        bool isEmpty() const { return _filename.isEmpty(); }
        QString getFilename() const { return _filename; }

    private:
        QString _filename;
    };

    class QueueEntry;

    class Preloader : public QObject
    {
        Q_OBJECT
    private:
        class PreloadTrack;

    public:
        Preloader(QObject* parent, PlayerQueue* queue, Resolver* resolver);

        ~Preloader();

        bool havePreloadedFileQuickCheck(uint queueId);
        PreloadedFile getPreloadedCacheFile(uint queueID);

        static void cleanupOldFiles();

    Q_SIGNALS:
        void trackPreloaded(uint queueId);

    private Q_SLOTS:
        void queueEntryAdded(qint32 offset, quint32 queueID);
        void queueEntryRemoved(qint32 offset, quint32 queueID);
        void queueEntryMoved(qint32 fromOffset, qint32 toOffset, quint32 queueID);
        void firstTrackInQueueChanged(int index, uint queueId);

        void scheduleFirstTrackCheck();
        void scheduleCheckForTracksToPreload();
        void checkFirstTrack();
        void checkForTracksToPreload();
        void scheduleCheckForCacheEntriesToDelete();
        void checkForCacheExpiration();

        void checkForJobsToStart();

        void preloadFinished(uint queueID, QString cacheFile);
        void preloadFailed(uint queueID);

    private:
        static const int PRELOAD_RANGE = 5;

        void checkToPreloadTrack(QSharedPointer<QueueEntry> entry);

        Future<QString, FailureType> preloadAsync(uint queueId, FileHash hash,
                                                  QString originalFilename);
        static ResultOrError<QString, FailureType> runPreload(uint queueId,
                                                              QString originalFilename);

        static QString tempFilename(uint queueId, QString extension);

        void doLock(uint queueId);
        void doUnlock(uint queueId);

        QHash<uint, uint> _lockedQueueIds;
        PlayerQueue* _queue;
        Resolver* _resolver;
        QHash<uint, PreloadTrack*> _tracksByQueueID;
        QList<uint> _tracksToPreload;
        QList<uint> _tracksRemoved;
        uint _jobsRunning;
        bool _firstTrackCheckTimerRunning;
        bool _preloadCheckTimerRunning;
        bool _cacheExpirationCheckTimerRunning;
    };
}
#endif
