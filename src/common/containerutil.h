/*
    Copyright (C) 2021-2024, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_CONTAINERUTIL_H
#define PMP_CONTAINERUTIL_H

#include <QHash>
#include <QList>
#include <QSet>
#include <QVector>

namespace PMP
{
    class ContainerUtil
    {
    public:
        template<typename T> static QVector<T> toVector(QSet<T> const& set,
                                                        int customReserveSize = -1)
        {
            QVector<T> v;
            v.reserve(customReserveSize >= 0 ? customReserveSize : set.size());

            for (T const& element : set)
                v.append(element);

            return v;
        }

        template<typename T> static QSet<T> toSet(QList<T> const& list)
        {
            QSet<T> set;
            set.reserve(list.size());

            for (T const& element : list)
                set << element;

            return set;
        }

        template<typename T> static void addToSet(QList<T> const& list, QSet<T>& set)
        {
            for (T const& element : list)
                set << element;
        }

        template<typename T>
        static void removeFromSet(QList<T> const& list, QSet<T>& set)
        {
            for (T const& element : list)
                set.remove(element);
        }

        template<typename K, typename V>
        static void removeKeysFromSet(QHash<K, V> const& hash, QSet<K>& set)
        {
            for (auto it = hash.constBegin(); it != hash.constEnd(); ++it)
                set.remove(it.key());
        }
    };
}
#endif
