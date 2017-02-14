/*
    Copyright (C) 2014-2017, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_CURRENTTRACKMONITOR_H
#define PMP_CURRENTTRACKMONITOR_H

#include "common/filehash.h"
#include "common/serverconnection.h"

#include <QObject>
#include <QString>
#include <QtGlobal>

QT_FORWARD_DECLARE_CLASS(QElapsedTimer)
QT_FORWARD_DECLARE_CLASS(QTimer)

namespace PMP {

    class CurrentTrackMonitor : public QObject {
        Q_OBJECT
    public:
        CurrentTrackMonitor(ServerConnection* connection);

        ServerConnection::PlayState state() const { return _state; }

    public slots:
        void seekTo(qint64 position);

    Q_SIGNALS:
        void playing(quint32 queueID, quint32 queueLength);
        void paused(quint32 queueID, quint32 queueLength);
        void stopped(quint32 queueLength);

        void queueLengthChanged(quint32 queueLength, int state);

        void trackProgress(quint32 queueID, quint64 position, int lengthSeconds);
        void trackProgress(quint64 position);

        void receivedTitleArtist(QString title, QString artist);
        void receivedPossibleFilename(QString name);

        void volumeChanged(int percentage);

    private slots:
        void connected();

        void receivedPlayerState(int state, quint8 volume, quint32 queueLength,
                                 quint32 nowPlayingQID, quint64 nowPlayingPosition);

        void receivedTrackInfo(quint32 queueID, QueueEntryType type, int lengthInSeconds,
                               QString title, QString artist);

        void receivedPossibleFilenames(quint32 queueID, QList<QString> names);

        void onVolumeChanged(int percentage);

        void onTimeout();

    private:
        static const int TIMER_INTERVAL = 40; /* 25 times per second */

        ServerConnection* _connection;
        ServerConnection::PlayState _state;
        int _volume;
        quint32 _queueLength;
        quint32 _nowPlayingQID;
        quint64 _nowPlayingPosition; // milliseconds
        bool _receivedTrackInfo;
        bool _receivedPossibleFilenames;
        int _nowPlayingLengthSeconds;
        QString _nowPlayingTitle;
        QString _nowPlayingArtist;
        QString _nowPlayingFilename;
        QTimer* _timer;
        QElapsedTimer* _elapsedTimer;
        quint64 _timerPosition;
    };
}
#endif
