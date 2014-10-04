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

#include "generator.h"

#include "queue.h"
#include "resolver.h"

#include <QTimer>

namespace PMP {

    Generator::Generator(Queue* queue, Resolver* resolver)
     : _queue(queue), _resolver(resolver), _enabled(false), _refillPending(false)
    {
        connect(_queue, SIGNAL(entryRemoved(quint32, quint32)), this, SLOT(queueEntryRemoved(quint32, quint32)));

        _refillPending = true;
        QTimer::singleShot(100, this, SLOT(checkAndRefillQueue()));
    }

    bool Generator::enabled() const {
        return _enabled;
    }

    void Generator::enable() {
        if (_enabled) return; /* enabled already */
        _enabled = true;
        emit enabledChanged(true);

        queueEntryRemoved(0, 0); /* force filling of the queue */
    }

    void Generator::disable() {
        if (!_enabled) return; /* disabled already */
        _enabled = false;
        emit enabledChanged(false);
    }

    void Generator::queueEntryRemoved(quint32, quint32) {
        if (_refillPending) return;
        _refillPending = true;
        QTimer::singleShot(100, this, SLOT(checkAndRefillQueue()));
    }

    void Generator::checkAndRefillQueue() {
        _refillPending = false;

        if (!_enabled) return;

        int tracksToGenerate = 1;
        uint queueLength = _queue->length();
        if (queueLength < 10) { tracksToGenerate = 10 - queueLength; }
        while (tracksToGenerate > 0) {
            tracksToGenerate--;
            HashID randomHash = _resolver->getRandom();
            if (randomHash.empty()) { break; /* nothing available */ }
            _queue->enqueue(randomHash);
        }
    }

}
