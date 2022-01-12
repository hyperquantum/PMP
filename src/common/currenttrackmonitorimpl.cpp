/*
    Copyright (C) 2020-2021, Kevin Andre <hyperquantum@gmail.com>

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

#include "currenttrackmonitorimpl.h"

#include "serverconnection.h"

#include <QtDebug>

namespace PMP {

    CurrentTrackMonitorImpl::CurrentTrackMonitorImpl(ServerConnection* connection)
     : CurrentTrackMonitor(connection),
       _connection(connection),
       _playerState(PlayerState::Unknown),
       _currentQueueId(0),
       _progressAtTimerStart(0),
       _currentHash(),
       _haveReceivedCurrentTrack(false),
       _currentTrackLengthMilliseconds(-1)
    {
        connect(
            _connection, &ServerConnection::connected,
            this, &CurrentTrackMonitorImpl::connected
        );
        connect(
            _connection, &ServerConnection::disconnected,
            this, &CurrentTrackMonitorImpl::connectionBroken
        );
        connect(
            _connection, &ServerConnection::receivedPlayerState,
            this, &CurrentTrackMonitorImpl::receivedPlayerState
        );
        connect(
            _connection, &ServerConnection::receivedQueueEntryHash,
            this, &CurrentTrackMonitorImpl::receivedQueueEntryHash
        );
        connect(
            _connection, &ServerConnection::receivedTrackInfo,
            this, &CurrentTrackMonitorImpl::receivedTrackInfo
        );
        connect(
            _connection, &ServerConnection::receivedPossibleFilenames,
            this, &CurrentTrackMonitorImpl::receivedPossibleFilenames
        );

        if (_connection->isConnected())
            connected();
    }

    PlayerState CurrentTrackMonitorImpl::playerState() const
    {
        return _playerState;
    }

    TriBool CurrentTrackMonitorImpl::isTrackPresent() const
    {
        if (!_haveReceivedCurrentTrack)
            return TriBool::unknown;

        return _currentQueueId > 0;
    }

    quint32 CurrentTrackMonitorImpl::currentQueueId() const
    {
        return _currentQueueId;
    }

    qint64 CurrentTrackMonitorImpl::currentTrackProgressMilliseconds() const
    {
        if (!_progressTimer.isValid())
            return -1;

        auto millisecondsSinceTimerStart = _progressTimer.elapsed();

        auto progress = _progressAtTimerStart + millisecondsSinceTimerStart;

        return progress;
    }

    FileHash CurrentTrackMonitorImpl::currentTrackHash() const
    {
        return _currentHash;
    }

    QString CurrentTrackMonitorImpl::currentTrackTitle() const
    {
        return _currentTrackTitle;
    }

    QString CurrentTrackMonitorImpl::currentTrackArtist() const
    {
        return _currentTrackArtist;
    }

    QString CurrentTrackMonitorImpl::currentTrackPossibleFilename() const
    {
        return _currentTrackPossibleFilename;
    }

    qint64 CurrentTrackMonitorImpl::currentTrackLengthMilliseconds() const
    {
        return _currentTrackLengthMilliseconds;
    }

    void CurrentTrackMonitorImpl::seekTo(qint64 positionInMilliseconds)
    {
        _connection->seekTo(_currentQueueId, positionInMilliseconds);
    }

    void CurrentTrackMonitorImpl::connected()
    {
        _connection->requestPlayerState();
    }

    void CurrentTrackMonitorImpl::connectionBroken()
    {
        _playerState = PlayerState::Unknown;

        _haveReceivedCurrentTrack = false;
        _currentQueueId = 0;

        _progressTimer.invalidate();
        _progressAtTimerStart = 0;

        clearTrackInfo();

        Q_EMIT currentTrackChanged();
    }

    void CurrentTrackMonitorImpl::receivedPlayerState(PlayerState state, quint8 volume,
                                                      quint32 queueLength,
                                                      quint32 nowPlayingQueueId,
                                                      quint64 nowPlayingPosition)
    {
        Q_UNUSED(volume)
        Q_UNUSED(queueLength)
        Q_UNUSED(nowPlayingPosition)

        _playerState = state;
        changeCurrentQueueId(nowPlayingQueueId);
        changeCurrentTrackPosition(nowPlayingPosition);
    }

    void CurrentTrackMonitorImpl::receivedQueueEntryHash(quint32 queueId,
                                                         QueueEntryType type,
                                                         FileHash hash)
    {
        if (queueId != _currentQueueId)
            return;

        if (type != QueueEntryType::Track)
            return;

        if (hash == _currentHash)
            return;

        _currentHash = hash;

        Q_EMIT currentTrackInfoChanged();
    }

    void CurrentTrackMonitorImpl::receivedTrackInfo(quint32 queueId, QueueEntryType type,
                                                    qint64 lengthMilliseconds,
                                                    QString title, QString artist)
    {
        if (queueId != _currentQueueId)
            return;

        if (type != QueueEntryType::Track)
            return;

        bool lengthChanged = lengthMilliseconds != _currentTrackLengthMilliseconds;
        bool tagsChanged = title != _currentTrackTitle || artist != _currentTrackArtist;

        if (!lengthChanged && !tagsChanged)
            return;

        _currentTrackLengthMilliseconds = lengthMilliseconds;
        _currentTrackTitle = title;
        _currentTrackArtist = artist;

        if (title.isEmpty() && artist.isEmpty()) {
            _connection->sendPossibleFilenamesRequest(queueId);
        }

        Q_EMIT currentTrackInfoChanged();

        if (lengthChanged)
            emitCalculatedTrackProgress();
    }

    void CurrentTrackMonitorImpl::receivedPossibleFilenames(quint32 queueId,
                                                            QList<QString> names)
    {
        if (queueId != _currentQueueId)
            return;

        QString longest;
        for (QString name : names) {
            if (name.size() >= longest.size())
                longest = name;
        }

        if (longest == _currentTrackPossibleFilename)
            return;

        _currentTrackPossibleFilename = longest;

        Q_EMIT currentTrackInfoChanged();
    }

    void CurrentTrackMonitorImpl::changeCurrentQueueId(quint32 queueId)
    {
        if (_haveReceivedCurrentTrack && _currentQueueId == queueId)
            return; /* no change */

        qDebug() << "current track changed to QID" << queueId;

        _currentQueueId = queueId;
        _haveReceivedCurrentTrack = true;

        clearTrackInfo();

        if (queueId > 0) {
            _connection->sendQueueEntryInfoRequest(queueId);

            QList<uint> ids { queueId };
            _connection->sendQueueEntryHashRequest(ids);
        }

        Q_EMIT currentTrackChanged();
        Q_EMIT currentTrackInfoChanged();
    }

    void CurrentTrackMonitorImpl::changeCurrentTrackPosition(qint64 positionMilliseconds)
    {
        auto queueId = _currentQueueId;

        if (queueId <= 0) {
            _progressTimer.invalidate();
            _progressAtTimerStart = 0;

            Q_EMIT trackProgressChanged(_playerState, 0, -1, -1);
            return;
        }

        auto length = _currentTrackLengthMilliseconds;

        _progressTimer.start();
        _progressAtTimerStart = positionMilliseconds;

        Q_EMIT trackProgressChanged(_playerState, queueId, positionMilliseconds, length);
    }

    void CurrentTrackMonitorImpl::emitCalculatedTrackProgress()
    {
        auto progress = currentTrackProgressMilliseconds();
        auto length = _currentTrackLengthMilliseconds;

        Q_EMIT trackProgressChanged(_playerState, _currentQueueId, progress, length);
    }

    void CurrentTrackMonitorImpl::clearTrackInfo()
    {
        _currentHash = FileHash();
        _currentTrackTitle.clear();
        _currentTrackArtist.clear();
        _currentTrackPossibleFilename.clear();
        _currentTrackLengthMilliseconds = -1;
    }

}
