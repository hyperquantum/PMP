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

#include <QtDebug>

namespace PMP::Client
{
    QueueHashesMonitorImpl::QueueHashesMonitorImpl(QObject* parent,
                                                   AbstractQueueMonitor* queueMonitor,
                                             QueueEntryInfoStorage* queueEntryInfoStorage)
     : QueueHashesMonitor(parent),
       _queueMonitor(queueMonitor),
       _queueEntryInfoStorage(queueEntryInfoStorage)
    {
        int queueCapacity = qMax(20, queueMonitor->queueLength() + 10);
        _queueIdToHash.reserve(queueCapacity);
        _hashToQueueIds.reserve(queueCapacity);

        connect(queueMonitor, &AbstractQueueMonitor::queueResetted,
                this, &QueueHashesMonitorImpl::onQueueResetted);

        connect(queueMonitor, &AbstractQueueMonitor::entriesReceived,
                this, &QueueHashesMonitorImpl::onEntriesReceived);

        connect(queueMonitor, &AbstractQueueMonitor::trackAdded,
                this, &QueueHashesMonitorImpl::onTrackAdded);

        connect(queueMonitor, &AbstractQueueMonitor::trackRemoved,
                this, &QueueHashesMonitorImpl::onTrackRemoved);

        connect(queueEntryInfoStorage, &QueueEntryInfoStorage::tracksChanged,
                this, &QueueHashesMonitorImpl::onTracksChanged);

        onEntriesReceived(-1 /* index not used */, queueMonitor->knownQueuePart());
    }

    bool QueueHashesMonitorImpl::isPresentInQueue(LocalHashId hashId) const
    {
        auto it = _hashToQueueIds.constFind(hashId);
        if (it == _hashToQueueIds.constEnd())
            return false;

        return it.value().isEmpty() == false;
    }

    void QueueHashesMonitorImpl::onQueueResetted(int queueLength)
    {
        Q_UNUSED(queueLength)
        qDebug() << "QueueHashesMonitor::onQueueResetted; length:" << queueLength;

        auto queueIds = _queueIdToHash.keys();
        for (auto queueId : queueIds)
        {
            disassociateHashFromQueueId(queueId, true);
        }

        int queueCapacity = queueLength + 10;
        _queueIdToHash.reserve(queueCapacity);
        _hashToQueueIds.reserve(queueCapacity);
    }

    void QueueHashesMonitorImpl::onEntriesReceived(int index, QList<quint32> entries)
    {
        Q_UNUSED(index)
        qDebug() << "QueueHashesMonitor::onEntriesReceived; index:" << index
                 << " entries:" << entries;

        for (auto queueId : entries)
        {
            onTrackAdded(-1 /* index not used */, queueId);
        }
    }

    void QueueHashesMonitorImpl::onTrackAdded(int index, quint32 queueId)
    {
        Q_UNUSED(index)
        qDebug() << "QueueHashesMonitor::onTrackAdded; index:" << index
                 << " QID:" << queueId;

        LocalHashId hashId;

        auto entryInfo = _queueEntryInfoStorage->entryInfoByQueueId(queueId);
        if (entryInfo != nullptr)
            hashId = entryInfo->hashId();

        associateHashWithQueueId(hashId, queueId, true);
    }

    void QueueHashesMonitorImpl::onTrackRemoved(int index, quint32 queueId)
    {
        Q_UNUSED(index)
        qDebug() << "QueueHashesMonitor::onTrackRemoved; index:" << index
                 << " QID:" << queueId;

        disassociateHashFromQueueId(queueId, true);
    }

    void QueueHashesMonitorImpl::onTracksChanged(QList<quint32> queueIds)
    {
        qDebug() << "QueueHashesMonitor::onTracksChanged; queueIds:" << queueIds;

        for (auto queueId : queueIds)
        {
            auto entryInfo = _queueEntryInfoStorage->entryInfoByQueueId(queueId);

            if (entryInfo == nullptr || entryInfo->hashId().isZero())
                disassociateHashFromQueueId(queueId, false);
            else
                associateHashWithQueueId(entryInfo->hashId(), queueId, false);
        }
    }

    void QueueHashesMonitorImpl::associateHashWithQueueId(LocalHashId hashId,
                                                          quint32 queueId, bool canAdd)
    {
        LocalHashId previousHash;
        bool previousHashPresenceChanged = false;
        bool hashPresenceChanged = false;

        auto it = _queueIdToHash.constFind(queueId);
        if (it != _queueIdToHash.constEnd())
        {
            if (it.value() == hashId)
                return;

            previousHash = it.value();
            if (!previousHash.isZero())
            {
                auto& otherHashQueueIds = _hashToQueueIds[previousHash];
                otherHashQueueIds.remove(queueId);
                previousHashPresenceChanged = otherHashQueueIds.isEmpty();
            }
        }

        if (canAdd || it != _queueIdToHash.constEnd())
        {
            _queueIdToHash.insert(queueId, hashId);

            if (!hashId.isZero())
            {
                auto& hashQueueIds = _hashToQueueIds[hashId];
                hashQueueIds.insert(queueId);
                hashPresenceChanged = hashQueueIds.size() == 1;
            }
        }

        if (previousHashPresenceChanged)
            Q_EMIT hashInQueuePresenceChanged(previousHash);

        if (hashPresenceChanged)
            Q_EMIT hashInQueuePresenceChanged(hashId);
    }

    void QueueHashesMonitorImpl::disassociateHashFromQueueId(quint32 queueId,
                                                             bool canRemove)
    {
        auto it = _queueIdToHash.constFind(queueId);

        if (it == _queueIdToHash.constEnd())
            return;

        auto previousHash = it.value();

        if (canRemove)
            _queueIdToHash.erase(it);
        else
            _queueIdToHash[queueId] = LocalHashId();

        if (!previousHash.isZero())
        {
            auto& queueIds = _hashToQueueIds[previousHash];
            queueIds.remove(queueId);
            bool hashPresenceChanged = queueIds.isEmpty();

            if (hashPresenceChanged)
                Q_EMIT hashInQueuePresenceChanged(previousHash);
        }
    }
}
