/*
    Copyright (C) 2016, Kevin Andre <hyperquantum@gmail.com>

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

#include "util.h"

#include <QAtomicInt>
#include <QDateTime>
#include <QtDebug>

namespace PMP {

    const QChar Util::EnDash = QChar(0x2013);
    const QChar Util::EAcute = QChar(0xE9);

    unsigned Util::getRandomSeed() {
        /* Because std::random_device seems to be not random at all on MINGW 4.8, we use
         * the system time and an incrementing counter instead. */

        static QAtomicInt counter;
        unsigned counterValue = counter.fetchAndAddOrdered(1);

        unsigned clockValue = (quint64)QDateTime::currentMSecsSinceEpoch() & 0xFFFFFFFFu;

        //qDebug() << "counterValue:" << counterValue << "  clockValue:" << clockValue;

        auto result = counterValue ^ clockValue;
        qDebug() << "Util::getRandomSeed returning" << result;
        return result;
    }

}
