/*
    Copyright (C) 2014, Kevin Andre <hyperquantum@gmail.com>

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

#include <cstdlib>
#include <QElapsedTimer>
#include <QtDebug>
#include <QTimer>

namespace PMP {

    CurrentTrackMonitor::CurrentTrackMonitor(ServerConnection* connection)
     : QObject(connection),
        _connection(connection), _state(ServerConnection::UnknownState), _volume(-1),
        _nowPlayingQID(0), _nowPlayingPosition(0),
        _receivedTrackInfo(false), _nowPlayingLengthSeconds(-1),
        _timer(new QTimer(this)), _elapsedTimer(new QElapsedTimer()), _timerPosition(0)
    {
        connect(_connection, SIGNAL(connected()), this, SLOT(connected()));
        connect(
            _connection,
            SIGNAL(receivedPlayerState(int, quint8, quint32, quint32, quint64)),
            this,
            SLOT(receivedPlayerState(int, quint8, quint32, quint32, quint64))
        );
        connect(
            _connection, SIGNAL(receivedTrackInfo(quint32, int, QString, QString)),
            this, SLOT(receivedTrackInfo(quint32, int, QString, QString))
        );
        connect(
            _connection, SIGNAL(volumeChanged(int)),
            this, SLOT(onVolumeChanged(int))
        );

        _timer->setInterval(TIMER_INTERVAL);
        connect(_timer, SIGNAL(timeout()), this, SLOT(onTimeout()));

        if (_connection->isConnected()) {
            connected();
        }
    }

    void CurrentTrackMonitor::connected() {
        _state = ServerConnection::UnknownState;
        _volume = -1;
        _nowPlayingQID = 0;
        _nowPlayingPosition = 0;
        _receivedTrackInfo = false;
        _nowPlayingLengthSeconds = -1;
        _nowPlayingTitle = "";
        _nowPlayingArtist = "";
        _timerPosition = 0;

        _connection->requestPlayerState();
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

        if (trackChanged && nowPlayingQID > 0) {
            _receivedTrackInfo = false;
            _nowPlayingLengthSeconds = -1;
            _nowPlayingTitle = "";
            _nowPlayingArtist = "";
            _connection->sendTrackInfoRequest(nowPlayingQID);
        }

        if (state == ServerConnection::Playing) {
            _state = state;
            _nowPlayingQID = nowPlayingQID;
            emit playing(nowPlayingQID);
        }

        if (state == ServerConnection::Paused) {
            _state = state;
            _nowPlayingQID = nowPlayingQID;
            emit paused(nowPlayingQID);
        }

        if (positionChanged || stateChanged) {
            /* re-align elapsed timer */
            _timerPosition = nowPlayingPosition;
            _elapsedTimer->start();
        }

        if (positionChanged) {
            _nowPlayingPosition = nowPlayingPosition;
            emit trackProgress(nowPlayingPosition);
        }

        if (state == ServerConnection::Stopped) {
            _state = state;
            _nowPlayingQID = 0;
            emit stopped();
        }

        if (volume != _volume) {
            _volume = volume;
            emit volumeChanged(_volume);
        }

        /* start or stop the elapsed timer */
        if (state == ServerConnection::Playing) {
            if (!_timer->isActive()) _timer->start();
        }
        else { /* not playing */
            if (_timer->isActive()) _timer->stop();
        }
    }

    void CurrentTrackMonitor::receivedTrackInfo(quint32 queueID, int lengthInSeconds,
                                                QString title, QString artist)
    {
        if (queueID != _nowPlayingQID) return;

        bool alreadyReceivedInfo = _receivedTrackInfo;
        _receivedTrackInfo = true;
        qDebug() << "CurrentTrackMonitor::receivedTrackInfo  artist:" << artist << " title:" << title;

        if (/*!alreadyReceivedInfo || */lengthInSeconds != _nowPlayingLengthSeconds) {
            _nowPlayingLengthSeconds = lengthInSeconds;
            if (lengthInSeconds >= 0) {
                emit trackProgress(queueID, _timerPosition, lengthInSeconds);
            }
        }

        if (!alreadyReceivedInfo /* send it even if it's empty */
            || title != _nowPlayingTitle || artist != _nowPlayingArtist)
        {
            _nowPlayingTitle = title;
            _nowPlayingArtist = artist;
            emit receivedTitleArtist(title, artist);
        }
    }

    void CurrentTrackMonitor::onVolumeChanged(int percentage) {
        if (percentage != _volume) {
            _volume = percentage;
            emit volumeChanged(_volume);
        }
    }

    void CurrentTrackMonitor::onTimeout() {
        if (_timerPosition >= _nowPlayingPosition + 1000) {
            /* oh boy, we are too far ahead */
            return;
        }

        _timerPosition = _nowPlayingPosition + _elapsedTimer->elapsed();
        emit trackProgress(_timerPosition);
    }

}
