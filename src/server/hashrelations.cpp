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

#include "hashrelations.h"

#include "common/containerutil.h"

namespace PMP
{
    HashRelations::HashRelations()
    {
        //
    }

    void HashRelations::loadEquivalences(QVector<QPair<uint, uint>> equivalences)
    {
        if (equivalences.isEmpty())
            return;

        QMutexLocker lock(&_mutex);

        for (auto pair : equivalences)
        {
            auto newEntry = QSharedPointer<Entry>::create();

            {
                auto const& entry = _hashes[pair.first];

                if (!entry.isNull())
                    newEntry->equivalentHashes.unite(entry->equivalentHashes);
            }

            {
                auto const& entry = _hashes[pair.second];

                if (!entry.isNull())
                    newEntry->equivalentHashes.unite(entry->equivalentHashes);
            }

            newEntry->equivalentHashes.unite(QSet<uint>({ pair.first, pair.second }));

            for (uint hash : qAsConst(newEntry->equivalentHashes))
            {
                _hashes[hash] = newEntry;
            }
        }
    }

    void HashRelations::markAsEquivalent(QVector<uint> hashes)
    {
        if (hashes.isEmpty())
            return;

        QMutexLocker lock(&_mutex);

        auto newEntry = QSharedPointer<Entry>::create();

        for (uint hash : hashes)
        {
            auto const& entry = _hashes[hash];

            if (!entry.isNull())
                newEntry->equivalentHashes.unite(entry->equivalentHashes);
        }

        newEntry->equivalentHashes.unite(QSet<uint>(hashes.begin(), hashes.end()));

        for (uint hash : qAsConst(newEntry->equivalentHashes))
        {
            _hashes[hash] = newEntry;
        }
    }

    bool HashRelations::areEquivalent(QVector<uint> hashes)
    {
        Q_ASSERT_X(hashes.size() >= 2,
                   "HashRelations::areEquivalent()",
                   "less than 2 hashes specified");

        QMutexLocker lock(&_mutex);

        auto const& entry = _hashes.value(hashes[0]);
        if (entry.isNull())
            return false;

        for (int i = 1; i < hashes.size(); ++i)
        {
            if (!entry->equivalentHashes.contains(hashes[i]))
                return false;
        }

        return true;
    }

    QVector<uint> HashRelations::getEquivalencyGroup(uint hashId)
    {
        QMutexLocker lock(&_mutex);

        auto const& entry = _hashes.value(hashId);
        if (entry.isNull())
            return { hashId };

        const auto ids = entry->equivalentHashes;
        return ContainerUtil::toVector(ids);
    }

    QSet<uint> HashRelations::getOtherHashesEquivalentTo(uint hashId)
    {
        QMutexLocker lock(&_mutex);

        auto const& entry = _hashes.value(hashId);
        if (entry.isNull())
            return {};

        auto result = entry->equivalentHashes;
        result.detach();
        result.remove(hashId);
        return result;
    }
}
