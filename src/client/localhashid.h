/*
    Copyright (C) 2023, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_CLIENT_LOCALHASHID_H
#define PMP_CLIENT_LOCALHASHID_H

#include <QDebug>
#include <QMetaType>

namespace PMP::Client
{
    class LocalHashId
    {
    public:
        constexpr LocalHashId() : _id(0) {}
        constexpr explicit LocalHashId(unsigned int id) : _id(id) {}

        constexpr unsigned int value() const { return _id; }
        constexpr bool isZero() const { return _id == 0; }

        LocalHashId& operator=(LocalHashId const&) = default;

        constexpr bool operator==(LocalHashId const& other) const
        {
            return _id == other._id;
        }

        constexpr bool operator!=(LocalHashId const& other) const
        {
            return !(*this == other);
        }

    private:
        unsigned int _id;
    };

    inline constexpr bool operator<(LocalHashId hashId1, LocalHashId hashId2)
    {
        return hashId1.value() < hashId2.value();
    }

    inline QDebug operator<<(QDebug debug, const PMP::Client::LocalHashId id)
    {
        debug << id.value();
        return debug;
    }

    constexpr inline uint qHash(LocalHashId const& id)
    {
        return id.value();
    }
}

Q_DECLARE_METATYPE(PMP::Client::LocalHashId)

#endif
