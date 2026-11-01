/*
    Copyright (C) 2023-2026, Kevin André <hyperquantum@gmail.com>

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

#ifndef PMP_CLIENT_QUEUEHASHESMONITOR_H
#define PMP_CLIENT_QUEUEHASHESMONITOR_H

#include "localtrackid.h"

#include <QHash>
#include <QList>
#include <QObject>
#include <QSet>

namespace PMP::Client
{
    class AbstractQueueMonitor;
    class QueueEntryInfoStorage;

    class QueueHashesMonitor : public QObject
    {
        Q_OBJECT
    protected:
        explicit QueueHashesMonitor(QObject* parent = nullptr) : QObject(parent) {}
    public:
        virtual ~QueueHashesMonitor() {}

        virtual bool isPresentInQueue(LocalTrackId hashId) const = 0;

    Q_SIGNALS:
        void hashInQueuePresenceChanged(LocalTrackId hashId);
    };

    class QueueHashesMonitorImpl : public QueueHashesMonitor
    {
        Q_OBJECT
    public:
        QueueHashesMonitorImpl(QObject* parent, AbstractQueueMonitor* queueMonitor,
                               QueueEntryInfoStorage* queueEntryInfoStorage);

        bool isPresentInQueue(LocalTrackId hashId) const override;

    private Q_SLOTS:
        void onQueueResetted(int queueLength);
        void onEntriesReceived(int index, QList<quint32> entries);
        void onTrackAdded(int index, quint32 queueId);
        void onTrackRemoved(int index, quint32 queueId);
        void onTracksChanged(QList<quint32> queueIds);

    private:
        void associateHashWithQueueId(LocalTrackId hashId, quint32 queueId, bool canAdd);
        void disassociateHashFromQueueId(quint32 queueId, bool canRemove);

        AbstractQueueMonitor* _queueMonitor;
        QueueEntryInfoStorage* _queueEntryInfoStorage;
        QHash<LocalTrackId, QSet<quint32>> _hashToQueueIds;
        QHash<quint32, LocalTrackId> _queueIdToHash;
    };
}
#endif
