/*
    Copyright (C) 2018-2022, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_TRACKTOSCROBBLE_H
#define PMP_TRACKTOSCROBBLE_H

#include <QDateTime>
#include <QString>

namespace PMP
{
    class TrackToScrobble
    {
    public:
        TrackToScrobble();
        virtual ~TrackToScrobble();

        virtual QDateTime timestamp() const = 0;
        virtual QString title() const = 0;
        virtual QString artist() const = 0;
        virtual QString album() const = 0;

        virtual void scrobbledSuccessfully() = 0;
        virtual void scrobbleIgnored() = 0;
    };
}
#endif
