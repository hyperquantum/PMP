/*
    Copyright (C) 2016-2020, Kevin Andre <hyperquantum@gmail.com>

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

#include <QByteArray>
#include <QChar>
#include <QString>

namespace PMP {

    class Util {
    public:
        static unsigned getRandomSeed();

        /*! The copyright symbol */
        static const QChar Copyright;

        /*! The EM DASH (U+2014) character */
        static const QChar EmDash;

        /*! The EN DASH (U+2013) character */
        static const QChar EnDash;

        /*! The LATIN SMALL LETTER E WITH ACUTE (U+E9) */
        static const QChar EAcute;

        /*! The LATIN SMALL LETTER E WITH DIAERESIS (U+00EB) */
        static const QChar EDiaeresis;

        /*! The FIGURE DASH (U+2012) character */
        static const QChar FigureDash;

        /*! The "GREATER-THAN OR EQUAL TO" symbol (U+2265) */
        static const QChar GreaterThanOrEqual;

        /*! The "LESS-THAN OR EQUAL TO" symbol (U+2264) */
        static const QChar LessThanOrEqual;

        /*! Pause symbol (U+23F8) */
        static const QChar PauseSymbol;

        /*! Play symbol (U+25B6) */
        static const QChar PlaySymbol;

        static QString secondsToHoursMinuteSecondsText(qint32 totalSeconds);
        static QString millisecondsToShortDisplayTimeText(qint64 milliseconds);
        static QString millisecondsToLongDisplayTimeText(qint64 milliseconds);

        static QString getCopyrightLine(bool mustBeAscii = true);

        static QByteArray generateZeroedMemory(int byteCount);

        static int compare(uint i1, uint i2) {
            if (i1 < i2) return -1;
            if (i1 > i2) return 1;
            return 0;
        }

        static int compare(int i1, int i2) {
            if (i1 < i2) return -1;
            if (i1 > i2) return 1;
            return 0;
        }

    private:
        Util();
    };
}
#endif
