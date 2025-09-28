/*
    Copyright (C) 2014-2025, Kevin André <hyperquantum@gmail.com>

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

#ifndef PMP_QUEUEENTRY_H
#define PMP_QUEUEENTRY_H

#include "common/audiodata.h"
#include "common/nullable.h"
#include "common/tagdata.h"

#include "queueentrykind.h"

#include <QDateTime>
#include <QSharedPointer>

#include <functional>

namespace PMP::Server
{
    class PlayerQueue;
    class Resolver;

    class QueueEntry
    {
    public:
        static QSharedPointer<QueueEntry> createBreak(uint queueId);
        static QSharedPointer<QueueEntry> createBarrier(uint queueId);
        static QSharedPointer<QueueEntry> createFromTrackId(uint queueId, uint trackId);
        static QSharedPointer<QueueEntry> createCopyOf(uint queueId,
                                               QSharedPointer<QueueEntry const> existing);

        ~QueueEntry();

        uint queueID() const { return _queueID; }
        QueueEntryKind kind() const { return _kind; }
        bool isTrack() const { return _kind == QueueEntryKind::Track; }

        Nullable<uint> trackId() const
        {
            if (_trackId > 0) return _trackId; else return null;
        }

        void setFilename(QString const& filename);
        Nullable<QString> filename() const;
        void invalidateFilename();

        void checkAudioData(Resolver& resolver);
        void checkTrackData(Resolver& resolver);

        /** Length in milliseconds. Is negative when unknown. */
        qint64 lengthInMilliseconds() const;

        QString artist() const;
        QString title() const;
        QString album() const;
        QString albumArtist() const;

        int& fileFinderBackoff() { return _fileFinderBackoff; }
        int& fileFinderFailedCount() { return _fileFinderFailedCount; }

        QDateTime started() const { return _started; }
        QDateTime ended() const { return _ended; }
        void setStartedNow();
        void setEndedNow();

    private:
        QueueEntry(uint queueId, uint trackId);
        QueueEntry(uint queueId, QSharedPointer<QueueEntry const> existing);
        QueueEntry(uint queueId, QueueEntryKind kind);

        friend class QSharedPointer<QueueEntry>;

        uint const _queueID;
        QueueEntryKind _kind;
        uint _trackId;
        //bool _fetchedAudioInfo;
        AudioData _audioInfo;
        QString _filename;
        bool _haveFilename;
        bool _fetchedTagData;
        TagData _tagData;
        int _fileFinderBackoff;
        int _fileFinderFailedCount;
        QDateTime _started;
        QDateTime _ended;
    };

    class QueueEntryCreators
    {
    public:
        static std::function<QSharedPointer<QueueEntry> (uint)> breakpoint()
        {
            return QueueEntry::createBreak;
        }

        static std::function<QSharedPointer<QueueEntry> (uint)> track(uint trackId)
        {
            return
                [trackId](uint queueId)
                {
                    return QueueEntry::createFromTrackId(queueId, trackId);
                };
        }

        static std::function<QSharedPointer<QueueEntry> (uint)> copyOf(
                                                QSharedPointer<QueueEntry const> existing)
        {
            return
                [existing](uint queueId)
                {
                    return QueueEntry::createCopyOf(queueId, existing);
                };
        }
    };
}
#endif
