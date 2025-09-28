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

#include "queueentry.h"

#include "resolver.h"

#include <QFileInfo>
#include <QtDebug>

namespace PMP::Server
{
    QueueEntry::QueueEntry(uint queueId, uint trackId)
     : _queueID(queueId),
       _kind(QueueEntryKind::Track),
       _trackId(trackId),
       _haveFilename(false),
       _fetchedTagData(false),
       _fileFinderBackoff(0),
       _fileFinderFailedCount(0)
    {
        //
    }

    QueueEntry::QueueEntry(uint queueId, QSharedPointer<QueueEntry const> existing)
     : _queueID(queueId),
       _kind(existing->_kind),
       _trackId(existing->_trackId),
       _audioInfo(existing->_audioInfo),
       _filename(existing->_filename),
       _haveFilename(existing->_haveFilename),
       _fetchedTagData(existing->_fetchedTagData),
       _tagData(existing->_tagData),
       _fileFinderBackoff(existing->_fileFinderBackoff),
       _fileFinderFailedCount(existing->_fileFinderFailedCount)
    {
        //
    }

    QueueEntry::QueueEntry(uint queueId, QueueEntryKind kind)
     : _queueID(queueId),
       _kind(kind),
       _trackId(0),
       _haveFilename(false),
       _fetchedTagData(false),
       _fileFinderBackoff(0),
       _fileFinderFailedCount(0)
    {
        //
    }

    QSharedPointer<QueueEntry> QueueEntry::createBreak(uint queueId)
    {
        return QSharedPointer<QueueEntry>::create(queueId, QueueEntryKind::Break);
    }

    QSharedPointer<QueueEntry> QueueEntry::createBarrier(uint queueId)
    {
        return QSharedPointer<QueueEntry>::create(queueId, QueueEntryKind::Barrier);
    }

    QSharedPointer<QueueEntry> QueueEntry::createFromTrackId(uint queueId, uint trackId)
    {
        return QSharedPointer<QueueEntry>::create(queueId, trackId);
    }

    QSharedPointer<QueueEntry> QueueEntry::createCopyOf(uint queueId,
                                                QSharedPointer<const QueueEntry> existing)
    {
        return QSharedPointer<QueueEntry>::create(queueId, existing);
    }

    QueueEntry::~QueueEntry()
    {
        //
    }

    void QueueEntry::setFilename(QString const& filename)
    {
        _filename = filename;
        _haveFilename = true;
    }

    Nullable<QString> QueueEntry::filename() const
    {
        if (_haveFilename)
            return _filename;

        return null;
    }

    void QueueEntry::invalidateFilename()
    {
        _haveFilename = false;
        _filename.clear();
    }

    void QueueEntry::checkAudioData(Resolver& resolver)
    {
        if (_trackId == 0) return;

        if (!_audioInfo.isComplete())
        {
            auto audioDataFound = resolver.findAudioData(_trackId);

            if (audioDataFound.hasValue())
                _audioInfo = audioDataFound.value();
        }
    }

    void QueueEntry::checkTrackData(Resolver& resolver)
    {
        if (_trackId == 0) return;

        checkAudioData(resolver);

        if (_fetchedTagData) return;

        auto tagDataFound = resolver.findTagData(_trackId);
        if (tagDataFound.hasValue())
        {
            _tagData = tagDataFound.value();
            _fetchedTagData = true;
        }
    }

    qint64 QueueEntry::lengthInMilliseconds() const
    {
        return _audioInfo.trackLengthMilliseconds();
    }

    QString QueueEntry::artist() const
    {
        return _tagData.artist();
    }

    QString QueueEntry::title() const
    {
        return _tagData.title();
    }

    QString QueueEntry::album() const
    {
        return _tagData.album();
    }

    QString QueueEntry::albumArtist() const
    {
        return _tagData.albumArtist();
    }

    void QueueEntry::setStartedNow()
    {
        _started = QDateTime::currentDateTimeUtc();
    }

    void QueueEntry::setEndedNow()
    {
        /* set 'started' equal to 'ended' if 'started' hasn't been set yet */
        if (_started.isNull())
        {
            _started = QDateTime::currentDateTimeUtc();
            _ended = _started;
            return;
        }

        _ended = QDateTime::currentDateTimeUtc();
    }
}
