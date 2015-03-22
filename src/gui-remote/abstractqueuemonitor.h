/*
    Copyright (C) 2015, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_ABSTRACTQUEUEMONITOR_H
#define PMP_ABSTRACTQUEUEMONITOR_H

#include <QList>
#include <QObject>
#include <QUuid>

namespace PMP {

    class AbstractQueueMonitor : public QObject {
        Q_OBJECT
    public:
        AbstractQueueMonitor(QObject* parent = 0);

        virtual QUuid serverUuid() const = 0;

        virtual int queueLength() const = 0;
        virtual quint32 queueEntry(int index) = 0;
        virtual QList<quint32> knownQueuePart() const = 0;

    Q_SIGNALS:
        void queueResetted(int queueLength);
        void entriesReceived(int index, QList<quint32> entries);
        void trackAdded(int index, quint32 queueID);
        void trackRemoved(int index, quint32 queueID);
        void trackMoved(int fromIndex, int toIndex, quint32 queueID);

    };
}
#endif
