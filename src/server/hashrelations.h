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

#ifndef PMP_HASHRELATIONS_H
#define PMP_HASHRELATIONS_H

#include "common/filehash.h"

#include <QHash>
#include <QMutex>
#include <QSharedPointer>
#include <QSet>
#include <QVector>

namespace PMP
{
    class HashRelations
    {
    public:
        HashRelations();

        void markAsEquivalent(QVector<uint> hashes);

        /// Return other hashes that are equivalent to the hash specified; the result does
        /// not include the original hash.
        QSet<uint> getOtherHashesEquivalentTo(uint hashId);

    private:
        struct Entry
        {
            QSet<uint> equivalentHashes;
        };

        QMutex _mutex;
        QHash<uint, QSharedPointer<Entry>> _hashes;
    };
}
#endif
