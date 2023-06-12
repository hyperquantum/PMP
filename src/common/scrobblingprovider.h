/*
    Copyright (C) 2019-2023, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_SCROBBLINGPROVIDER_H
#define PMP_SCROBBLINGPROVIDER_H

#include <QDebug>
#include <QHashFunctions>

namespace PMP
{
    enum class ScrobblingProvider
    {
        Unknown = 0,
        LastFm,
    };

    QString toString(ScrobblingProvider provider);

    QDebug operator<<(QDebug debug, ScrobblingProvider provider);

    inline uint qHash(ScrobblingProvider provider, uint seed)
    {
        return ::qHash(static_cast<int>(provider), seed);
    }
}

Q_DECLARE_METATYPE(PMP::ScrobblingProvider)

#endif
