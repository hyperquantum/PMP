/*
    Copyright (C) 2023, Kevin Andre <hyperquantum@gmail.com>

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

#include "queuehashesmonitor.h"

#include "abstractqueuemonitor.h"
#include "queueentryinfostorage.h"

namespace PMP::Client
{
    QueueHashesMonitor::QueueHashesMonitor(QObject* parent,
                                           AbstractQueueMonitor* queueMonitor,
                                           QueueEntryInfoStorage* queueEntryInfoStorage)
     : QObject(parent),
       _queueMonitor(queueMonitor),
       _queueEntryInfoStorage(queueEntryInfoStorage)
    {
        connect(queueMonitor, &AbstractQueueMonitor::trackAdded,
                this, &QueueHashesMonitor::onTrackAdded);

        connect(queueMonitor, &AbstractQueueMonitor::trackRemoved,
                this, &QueueHashesMonitor::onTrackRemoved);

        connect(queueEntryInfoStorage, &QueueEntryInfoStorage::tracksChanged,
                this, &QueueHashesMonitor::onTracksChanged);
    }

    bool QueueHashesMonitor::isPresentInQueue(FileHash hash) const
    {
        auto it = _hashToQueueIds.constFind(hash);
        if (it == _hashToQueueIds.constEnd())
            return false;

        return it.value().isEmpty() == false;
    }

    void QueueHashesMonitor::onQueueResetted(int queueLength)
    {
        Q_UNUSED(queueLength)
        // we'll do nothing for now
    }

    void QueueHashesMonitor::onEntriesReceived(int index, QList<quint32> entries)
    {
        Q_UNUSED(index)

        onTracksChanged(entries);
    }

    void QueueHashesMonitor::onTrackAdded(int index, quint32 queueId)
    {
        Q_UNUSED(index)

        auto entryInfo = _queueEntryInfoStorage->entryInfoByQueueId(queueId);
        if (entryInfo == nullptr)
            return;

        auto hash = entryInfo->hash();
        if (hash.isNull())
            return;

        associateHashWithQueueId(hash, queueId);
    }

    void QueueHashesMonitor::onTrackRemoved(int index, quint32 queueId)
    {
        Q_UNUSED(index)

        disassociateHashFromQueueId(queueId);
    }

    void QueueHashesMonitor::onTracksChanged(QList<quint32> queueIds)
    {
        for (auto queueId : queueIds)
        {
            auto entryInfo = _queueEntryInfoStorage->entryInfoByQueueId(queueId);

            if (entryInfo == nullptr || entryInfo->hash().isNull())
                disassociateHashFromQueueId(queueId);
            else
                associateHashWithQueueId(entryInfo->hash(), queueId);
        }
    }

    void QueueHashesMonitor::associateHashWithQueueId(const FileHash& hash,
                                                      quint32 queueId)
    {
        if (hash.isNull())
            return;

        FileHash previousHash;
        bool previousHashPresenceChanged = false;

        auto it = _queueIdToHash.constFind(queueId);

        if (it != _queueIdToHash.constEnd())
        {
            if (it.value() == hash)
                return;

            previousHash = it.value();
            auto& otherHashQueueIds = _hashToQueueIds[previousHash];
            otherHashQueueIds.remove(queueId);

            if (otherHashQueueIds.isEmpty())
                previousHashPresenceChanged = true;
        }

        _queueIdToHash.insert(queueId, hash);
        auto& hashQueueIds = _hashToQueueIds[hash];
        hashQueueIds.insert(queueId);
        bool hashPresenceChanged = hashQueueIds.size() == 1;

        if (previousHashPresenceChanged)
            Q_EMIT hashInQueuePresenceChanged(previousHash);

        if (hashPresenceChanged)
            Q_EMIT hashInQueuePresenceChanged(hash);
    }

    void QueueHashesMonitor::disassociateHashFromQueueId(quint32 queueId)
    {
        auto it = _queueIdToHash.constFind(queueId);

        if (it == _queueIdToHash.constEnd())
            return;

        auto hash = it.value();

        _queueIdToHash.erase(it);

        auto& queueIds = _hashToQueueIds[hash];
        queueIds.remove(queueId);
        bool hashPresenceChanged = queueIds.isEmpty();

        if (hashPresenceChanged)
            Q_EMIT hashInQueuePresenceChanged(hash);
    }
}
