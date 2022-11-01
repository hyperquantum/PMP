/*
    Copyright (C) 2022, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_HASHSTATS_H
#define PMP_HASHSTATS_H

#include "common/filehash.h"

#include "trackstats.h"

namespace PMP
{
    class HashStats
    {
    public:
        HashStats() {}

        HashStats(FileHash const& hash, TrackStats const& stats)
         : _hash(hash), _stats(stats)
        {
            //
        }

        FileHash const& hash() const { return _hash; }
        TrackStats const& stats() const { return _stats; }

    private:
        FileHash _hash;
        TrackStats _stats;
    };
}

Q_DECLARE_METATYPE(PMP::HashStats)

#endif
