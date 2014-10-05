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

#ifndef PMP_GENERATOR_H
#define PMP_GENERATOR_H

#include <QObject>

namespace PMP {

    class HashID;
    class Queue;
    class Resolver;

    class Generator : public QObject {
        Q_OBJECT
    public:
        Generator(Queue* queue, Resolver* resolver);

        bool enabled() const;

    public slots:
        void enable();
        void disable();

    Q_SIGNALS:
        void enabledChanged(bool enabled);

    private slots:
        void queueEntryRemoved(quint32, quint32);

        void checkAndRefillQueue();

    private:
        bool satisfiesFilters(const HashID& hash);

        Queue* _queue;
        Resolver* _resolver;
        bool _enabled;
        bool _refillPending;
    };
}
#endif
