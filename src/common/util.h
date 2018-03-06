/*
    Copyright (C) 2016-2018, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_UTIL_H
#define PMP_UTIL_H

#include <QChar>
#include <QString>

namespace PMP {

    class Util {
    public:
        static unsigned getRandomSeed();

        /*! The copyright symbol */
        static const QChar Copyright;

        /*! The EN DASH (U+2013) character. */
        static const QChar EnDash;

        /*! The LATIN SMALL LETTER E WITH ACUTE (U+E9) */
        static const QChar EAcute;

        /*! The LATIN SMALL LETTER E WITH DIAERESIS (U+00EB) */
        static const QChar EDiaeresis;

        static QString secondsToHoursMinuteSecondsText(qint32 totalSeconds);

        static QString getCopyrightLine(bool mustBeAscii = true);

    private:
        Util();
    };
}
#endif
