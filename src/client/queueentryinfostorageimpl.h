/*
    Copyright (C) 2014-2023, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_QUEUEENTRYINFOSTORAGEIMPL_H
#define PMP_QUEUEENTRYINFOSTORAGEIMPL_H

#include "queueentryinfostorage.h"

#include <QHash>
#include <QSet>

namespace PMP::Client
{
    class ServerConnection;

    class QueueEntryInfoStorageImpl : public QueueEntryInfoStorage
    {
        Q_OBJECT
    public:
        explicit QueueEntryInfoStorageImpl(ServerConnection* connection);

        QueueEntryInfo* entryInfoByQueueId(quint32 queueId) override;

        void fetchEntry(quint32 queueId) override;
        void fetchEntries(QList<quint32> queueIds) override;
        void refetchEntries(QList<quint32> queueIds) override;

    public Q_SLOTS:
        void dropInfoFor(quint32 queueId) override;

    private Q_SLOTS:
        void connected();
        void connectionBroken();

        void receivedQueueEntryHash(quint32 queueID, QueueEntryType type,
                                    LocalHashId hashId);
        void receivedTrackInfo(quint32 queueID, QueueEntryType type,
                               qint64 lengthMilliseconds, QString title, QString artist);
        void receivedPossibleFilenames(quint32 queueID, QList<QString> names);

        void enqueueTrackChangeNotification(quint32 queueID);
        void emitTracksChangedSignal();

    private:
        void sendInfoRequest(quint32 queueId);
        void sendHashRequest(quint32 queueId);

        ServerConnection* _connection;
        QHash<quint32, QueueEntryInfo*> _entries;
        QSet<quint32> _trackChangeNotificationsPending;
        QSet<quint32> _infoRequestsSent;
        QSet<quint32> _hashRequestsSent;
    };
}
#endif
