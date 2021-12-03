/*
    Copyright (C) 2020-2021, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_STARTSTOPEVENTSTATUS_H
#define PMP_STARTSTOPEVENTSTATUS_H

#include <QMetaType>
#include <QtGlobal>

namespace PMP
{
    enum class StartStopEventStatus : quint8
    {
        Undefined = 0,
        StatusUnchangedNotActive = 1,
        StatusUnchangedActive = 2,
        StatusChangedToActive = 3,
        StatusChangedToNotActive = 4,
    };

    namespace Common
    {
        bool isValidStartStopEventStatus(quint8 status);
        bool isActive(StartStopEventStatus status);
        bool isChange(StartStopEventStatus status);
        StartStopEventStatus createUnchangedStartStopEventStatus(bool active);
        StartStopEventStatus createChangedStartStopEventStatus(bool active);
    }
}

Q_DECLARE_METATYPE(PMP::StartStopEventStatus)

#endif
