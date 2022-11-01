/*
    Copyright (C) 2016-2022, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_UNICODECHARS_H
#define PMP_UNICODECHARS_H

#include <QChar>

namespace PMP
{
    namespace UnicodeChars
    {
        /*! The copyright symbol */
        [[maybe_unused]] constexpr QChar copyright = QChar(0xA9);

        /*! The EM DASH (U+2014) character */
        [[maybe_unused]] constexpr QChar emDash = QChar(0x2014);

        /*! The EN DASH (U+2013) character */
        [[maybe_unused]] constexpr QChar enDash = QChar(0x2013);

        /*! The LATIN SMALL LETTER E WITH ACUTE (U+E9) */
        [[maybe_unused]] constexpr QChar eAcute = QChar(0xE9);

        /*! The LATIN SMALL LETTER E WITH DIAERESIS (U+00EB) */
        [[maybe_unused]] constexpr QChar eDiaeresis = QChar(0xEB);

        /*! The FIGURE DASH (U+2012) character */
        [[maybe_unused]] constexpr QChar figureDash = QChar(0x2012);

        /*! The "GREATER-THAN OR EQUAL TO" symbol (U+2265) */
        [[maybe_unused]] constexpr QChar greaterThanOrEqual = QChar(0x2265);

        /*! The "LESS-THAN OR EQUAL TO" symbol (U+2264) */
        [[maybe_unused]] constexpr QChar lessThanOrEqual = QChar(0x2264);

        /*! Pause symbol (U+23F8) */
        [[maybe_unused]] constexpr QChar pauseSymbol = QChar(0x23F8);

        /*! Play symbol (U+25B6) */
        [[maybe_unused]] constexpr QChar playSymbol = QChar(0x25B6);
    }
}
#endif
