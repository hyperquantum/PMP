/*
    Copyright (C) 2020, Kevin Andre <hyperquantum@gmail.com>

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

#include "powermanagement.h"

#include <QString>
#include <QtDebug>
#include <QtGlobal>
#include <QTimer>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace PMP {

    PowerManagement::PowerManagement(QObject* parent)
     : QObject(parent), _keepDisplayActive(false)
    {
        //
    }

    bool PowerManagement::getKeepDisplayActive() const {
        return _keepDisplayActive;
    }

    void PowerManagement::setKeepDisplayActive(bool keepActive) {
        if (keepActive == _keepDisplayActive)
            return;

        _keepDisplayActive = keepActive;

        QTimer::singleShot(0, this, &PowerManagement::updateState);
    }

    bool PowerManagement::isPlatformSupported() const {
#ifdef Q_OS_WIN
        return true;
#else
        return false;
#endif
    }

    void PowerManagement::updateState() {
#ifdef Q_OS_WIN
        auto oldValue =
            _keepDisplayActive
                ? SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED)
                : SetThreadExecutionState(ES_CONTINUOUS);

        if (!oldValue)
            qWarning() << "SetThreadExecutionState call failed";

        qDebug() << "SetThreadExecutionState returned"
                 << QString::number(oldValue, 16) << "(hex)";
#else
        qWarning() << "Platform not supported for power management; call has no effect";
#endif
    }

}
