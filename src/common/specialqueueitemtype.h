/*
    Copyright (C) 2021, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_SPECIALQUEUEITEMTYPE_H
#define PMP_SPECIALQUEUEITEMTYPE_H

#include <QtDebug>

namespace PMP
{

    enum class SpecialQueueItemType { Break, Barrier };

    inline QDebug operator<<(QDebug debug, SpecialQueueItemType type)
    {
        switch (type)
        {
        case SpecialQueueItemType::Break:
            debug << "SpecialQueueItemType::Break";
            return debug;

        case SpecialQueueItemType::Barrier:
            debug << "SpecialQueueItemType::Barrier";
            return debug;
        }

        debug << int(type);
        return debug;
    }
}

Q_DECLARE_METATYPE(PMP::SpecialQueueItemType)

#endif
