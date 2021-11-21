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

#include "startstopeventstatus.h"

namespace PMP
{
    namespace Common
    {
        bool isValidStartStopEventStatus(quint8 status)
        {
            switch (status)
            {
                case int(StartStopEventStatus::Undefined):
                    // we do not consider this a valid status
                    return false;

                case int(StartStopEventStatus::StatusUnchangedNotActive):
                case int(StartStopEventStatus::StatusUnchangedActive):
                case int(StartStopEventStatus::StatusChangedToActive):
                case int(StartStopEventStatus::StatusChangedToNotActive):
                    return true;
            }

            return false;
        }

        bool isActive(StartStopEventStatus status)
        {
            return status == StartStopEventStatus::StatusChangedToActive
                    || status == StartStopEventStatus::StatusUnchangedActive;
        }

        bool isChange(StartStopEventStatus status)
        {
            return status == StartStopEventStatus::StatusChangedToActive
                    || status == StartStopEventStatus::StatusChangedToNotActive;
        }

        StartStopEventStatus createUnchangedStartStopEventStatus(bool active)
        {
            return active
                    ? StartStopEventStatus::StatusUnchangedActive
                    : StartStopEventStatus::StatusUnchangedNotActive;
        }

        StartStopEventStatus createChangedStartStopEventStatus(bool active)
        {
            return active
                    ? StartStopEventStatus::StatusChangedToActive
                    : StartStopEventStatus::StatusChangedToNotActive;
        }

    }
}
