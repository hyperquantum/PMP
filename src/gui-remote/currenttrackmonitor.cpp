/*
    Copyright (C) 2014-2020, Kevin Andre <hyperquantum@gmail.com>

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

#include "currenttrackmonitor.h"

#include "common/serverconnection.h"

#include <QElapsedTimer>
#include <QtDebug>
#include <QTimer>

namespace PMP {

    CurrentTrackMonitor::CurrentTrackMonitor(ServerConnection* connection)
     : QObject(connection),
        _connection(connection), _state(ServerConnection::UnknownState), _volume(-1),
        _queueLength(0), _nowPlayingQID(0), _nowPlayingPosition(0),
        _receivedTrackInfo(false), _receivedPossibleFilenames(false),
        _nowPlayingLengthSeconds(-1),
        _timer(new QTimer(this)), _elapsedTimer(new QElapsedTimer()), _timerPosition(0)
    {
        connect(
            _connection, &ServerConnection::connected,
            this, &CurrentTrackMonitor::connected
        );
        connect(
            _connection, &ServerConnection::receivedPlayerState,
            this, &CurrentTrackMonitor::receivedPlayerState
        );
        connect(
            _connection, &ServerConnection::receivedQueueEntryHash,
            this, &CurrentTrackMonitor::receivedQueueEntryHash
        );
        connect(
            _connection, &ServerConnection::receivedTrackInfo,
            this, &CurrentTrackMonitor::receivedTrackInfo
        );
        connect(
            _connection, &ServerConnection::receivedPossibleFilenames,
            this, &CurrentTrackMonitor::receivedPossibleFilenames
        );
        connect(
            _connection, &ServerConnection::volumeChanged,
            this, &CurrentTrackMonitor::onVolumeChanged
        );

        _timer->setInterval(TIMER_INTERVAL);
        connect(_timer, &QTimer::timeout, this, &CurrentTrackMonitor::onTimeout);

        if (_connection->isConnected()) {
            connected();
        }
    }

    void CurrentTrackMonitor::connected() {
        _state = ServerConnection::UnknownState;
        _volume = -1;
        _queueLength = 0;
        _nowPlayingQID = 0;
        _nowPlayingPosition = 0;
        clearTrackInfo();
        _timerPosition = 0;

        _connection->requestPlayerState();
    }

    void CurrentTrackMonitor::seekTo(qint64 position) {
        _connection->seekTo(_nowPlayingQID, position);
    }

    void CurrentTrackMonitor::receivedPlayerState(int s, quint8 volume,
                                                  quint32 queueLength,
                                                  quint32 nowPlayingQID,
                                                  quint64 nowPlayingPosition)
    {
        ServerConnection::PlayState state = (ServerConnection::PlayState)s;

        bool stateChanged = state != _state;
        bool trackChanged = nowPlayingQID != _nowPlayingQID;
        bool positionChanged = nowPlayingPosition != _nowPlayingPosition;

        _state = state;
        _nowPlayingQID = nowPlayingQID;
        _nowPlayingPosition = nowPlayingPosition;

        if (trackChanged) {
            clearTrackInfo();

            if (nowPlayingQID > 0) {
                _connection->sendQueueEntryInfoRequest(nowPlayingQID);

                QList<uint> ids { nowPlayingQID };
                _connection->sendQueueEntryHashRequest(ids);
            }
        }

        if (state == ServerConnection::Playing) {
            Q_EMIT playing(nowPlayingQID, queueLength);
        }

        if (state == ServerConnection::Paused) {
            Q_EMIT paused(nowPlayingQID, queueLength);
        }

        if (positionChanged || stateChanged) {
            /* re-align elapsed timer */
            _timerPosition = nowPlayingPosition;
            _elapsedTimer->start();
        }

        if (positionChanged) {
            Q_EMIT trackProgress(nowPlayingPosition);
        }

        if (state == ServerConnection::Stopped) {
            Q_EMIT stopped(queueLength);
        }

        if (trackChanged) {
            Q_EMIT currentTrackChanged();
        }

        if (queueLength != _queueLength) {
            _queueLength = queueLength;
            Q_EMIT queueLengthChanged(queueLength, state);
        }

        if (volume != _volume) {
            _volume = volume;
            Q_EMIT volumeChanged(_volume);
        }

        /* start or stop the elapsed timer */
        if (state == ServerConnection::Playing) {
            if (!_timer->isActive()) _timer->start();
        }
        else { /* not playing */
            if (_timer->isActive()) _timer->stop();
        }
    }

    void CurrentTrackMonitor::receivedQueueEntryHash(quint32 queueId, QueueEntryType type,
                                                     FileHash hash)
    {
        if (queueId != _nowPlayingQID) return;

        if (type != QueueEntryType::Track) return;

        qDebug() << "received hash for current track (QID" << queueId << "):" << hash;

        if (_nowPlayingHash != hash) {
            _nowPlayingHash = hash;
            Q_EMIT receivedHash();
        }
    }

    void CurrentTrackMonitor::receivedTrackInfo(quint32 queueID, QueueEntryType type,
                                                int lengthInSeconds,
                                                QString title, QString artist)
    {
        Q_UNUSED(type)

        if (queueID != _nowPlayingQID) return;

        bool alreadyReceivedInfo = _receivedTrackInfo;
        _receivedTrackInfo = true;
        qDebug() << "CurrentTrackMonitor::receivedTrackInfo:  artist:" << artist
                 << " title:" << title;

        if (/*!alreadyReceivedInfo || */lengthInSeconds != _nowPlayingLengthSeconds) {
            _nowPlayingLengthSeconds = lengthInSeconds;
            if (lengthInSeconds >= 0) {
                Q_EMIT trackProgress(queueID, _timerPosition, lengthInSeconds);
            }
        }

        if (!alreadyReceivedInfo /* send it even if it's empty */
            || title != _nowPlayingTitle || artist != _nowPlayingArtist)
        {
            _nowPlayingTitle = title;
            _nowPlayingArtist = artist;
            Q_EMIT receivedTitleArtist(title, artist);

            if (!alreadyReceivedInfo && title.trimmed() == ""
                && !_receivedPossibleFilenames)
            {
                _connection->sendPossibleFilenamesRequest(queueID);
            }
        }
    }

    void CurrentTrackMonitor::receivedPossibleFilenames(quint32 queueID,
                                                        QList<QString> names)
    {
        if (queueID != _nowPlayingQID) return;

        QString longest;
        Q_FOREACH(QString name, names) {
            if (name.size() > longest.size()) longest = name;
        }

        if (_nowPlayingFilename != longest) {
            _nowPlayingFilename = longest;
            Q_EMIT receivedPossibleFilename(longest);
        }
    }

    void CurrentTrackMonitor::onVolumeChanged(int percentage) {
        if (percentage != _volume) {
            _volume = percentage;
            Q_EMIT volumeChanged(_volume);
        }
    }

    void CurrentTrackMonitor::onTimeout() {
        if (_timerPosition >= _nowPlayingPosition + 1000) {
            /* oh boy, we are too far ahead */
            return;
        }

        _timerPosition = _nowPlayingPosition + _elapsedTimer->elapsed();
        Q_EMIT trackProgress(_timerPosition);
    }

    void CurrentTrackMonitor::clearTrackInfo()
    {
        _receivedTrackInfo = false;
        _receivedPossibleFilenames = false;
        _nowPlayingLengthSeconds = -1;
        _nowPlayingHash = FileHash();
        _nowPlayingTitle = "";
        _nowPlayingArtist = "";
        _nowPlayingFilename = "";
    }
}
