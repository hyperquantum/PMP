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

#ifndef PMP_QUEUEMONITOR_H
#define PMP_QUEUEMONITOR_H

#include <QHash>
#include <QList>
#include <QObject>

namespace PMP {

    class ServerConnection;

    class TrackMonitor : public QObject {
        Q_OBJECT
    public:
        TrackMonitor(quint32 queueID/*, int position*/);

        quint32 queueID() const { return _queueID; }

        /*int queuePosition() const { return _position; }
        void setQueuePosition(int position);*/

        int lengthInSeconds();
        QString title();
        QString artist();

        bool setInfo(int lengthInSeconds, QString const& title, QString const& artist);

    Q_SIGNALS:
        void pleaseAskForMyInfo(quint32 queueID);
        void infoChanged();

    private:
        quint32 _queueID;
        //int _position;
        bool _infoRequestedAlready;
        int _lengthSeconds;
        QString _title;
        QString _artist;
    };

    class QueueMonitor : public QObject {
        Q_OBJECT

    public:
        QueueMonitor(QObject* parent, ServerConnection* connection);

        uint queueLength() const { return _queueLength; }
        quint32 queueEntry(int index) const;
        TrackMonitor* trackAtPosition(int index);
        TrackMonitor* trackFromID(quint32 queueID);

    Q_SIGNALS:
        //void queueLengthChanged(int newLength);
        void tracksInserted(int firstIndex, int lastIndex);
        void tracksRemoved(int firstIndex, int lastIndex);
        void tracksChanged(int firstIndex, int lastIndex);

    private slots:
        void connected();
        void receivedQueueContents(int queueLength, int startOffset, QList<quint32> queueIDs);
        void queueEntryRemoved(quint32 offset, quint32 queueID);
        //void trackIsAtPosition(quint32 queueID, int index);
        void receivedTrackInfo(quint32 queueID, int lengthInSeconds, QString title, QString artist);
        void emitTracksChangedSignal();

    private:
        ServerConnection* _connection;
        int _queueLength;
        int _queueLengthSent;
        int _queueRequestedUpTo;
        QList<quint32> _queue; // no need for a QQueue, a QList will suffice
        QHash<quint32, TrackMonitor*> _tracks;
        //QHash<quint32, int> _idToPosition;
        bool _trackChangeEventPending;
    };
}
#endif