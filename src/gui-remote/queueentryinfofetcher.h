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

#ifndef PMP_QUEUEENTRYINFOFETCHER_H
#define PMP_QUEUEENTRYINFOFETCHER_H

#include "common/filehash.h"
#include "common/queueentrytype.h"

#include "abstractqueuemonitor.h"

#include <QDateTime>
#include <QHash>
#include <QSet>

namespace PMP {

    class AbstractQueueMonitor;
    class ServerConnection;

    class QueueEntryInfo : public QObject {
        Q_OBJECT
    public:
        QueueEntryInfo(quint32 queueID);

        quint32 queueID() const { return _queueID; }

        QueueEntryType type() const { return _type; }
        const FileHash& hash() const { return _hash; }
        int lengthInSeconds() const { return _lengthSeconds; }
        QString artist() const { return _artist; }
        QString title() const { return _title; }

        QString informativeFilename() const { return _informativeFilename; }

        void setHash(QueueEntryType type, const FileHash& hash);
        void setInfo(QueueEntryType type, int lengthInSeconds,
                     QString const& title, QString const& artist);
        bool setPossibleFilenames(QList<QString> const& names);

    private:
        quint32 _queueID;
        QueueEntryType _type;
        FileHash _hash;
        int _lengthSeconds;
        QString _title;
        QString _artist;
        QString _informativeFilename;
    };

    class QueueEntryInfoFetcher : public QObject {
        Q_OBJECT
    public:
        QueueEntryInfoFetcher(QObject* parent, AbstractQueueMonitor* monitor,
                              ServerConnection* connection);

        QueueEntryInfo* entryInfoByQID(quint32 queueID);

    public Q_SLOTS:
        void dropInfoFor(quint32 queueId);

    Q_SIGNALS:
        void tracksChanged(QList<quint32> queueIDs);

    private Q_SLOTS:
        void connected();

        void receivedQueueEntryHash(quint32 queueID, QueueEntryType type, FileHash hash);
        void receivedTrackInfo(quint32 queueID, QueueEntryType type, int lengthInSeconds,
                               QString title, QString artist);
        void receivedPossibleFilenames(quint32 queueID, QList<QString> names);

        void queueResetted(int queueLength);
        void entriesReceived(int index, QList<quint32> entries);
        void trackAdded(int index, quint32 queueID);
        void trackMoved(int fromIndex, int toIndex, quint32 queueID);

        void enqueueTrackChangeNotification(quint32 queueID);
        void emitTracksChangedSignal();

    private:
        void sendRequest(quint32 queueID);
        void sendHashRequest(quint32 queueId);

        static const int initialQueueFetchLength = 10;

        AbstractQueueMonitor* _monitor;
        ServerConnection* _connection;
        QHash<quint32, QueueEntryInfo*> _entries;
        QSet<quint32> _trackChangeNotificationsPending;
        QSet<quint32> _infoRequestsSent;
        QSet<quint32> _hashRequestsSent;
    };
}
#endif
