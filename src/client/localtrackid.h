/*
    Copyright (C) 2023-2026, Kevin André <hyperquantum@gmail.com>

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

#ifndef PMP_CLIENT_LOCALTRACKID_H
#define PMP_CLIENT_LOCALTRACKID_H

#include <QDebug>
#include <QMetaType>
#include <QtGlobal>
//#include <QtTypes> <<-- for uint

namespace PMP::Client
{
    class LocalTrackId
    {
    public:
        constexpr LocalTrackId() : _id(0) {}
        constexpr explicit LocalTrackId(uint id) : _id(id) {}

        constexpr uint value() const { return _id; }
        constexpr bool isZero() const { return _id == 0; }

        LocalTrackId& operator=(LocalTrackId const&) = default;

        constexpr bool operator==(LocalTrackId const& other) const
        {
            return _id == other._id;
        }

    private:
        uint _id;
    };

    inline constexpr bool operator<(LocalTrackId hashId1, LocalTrackId hashId2)
    {
        return hashId1.value() < hashId2.value();
    }

    inline QDebug operator<<(QDebug debug, const LocalTrackId id)
    {
        debug << id.value();
        return debug;
    }

    constexpr inline uint qHash(LocalTrackId const& id, size_t seed)
    {
        return qHashMulti(seed, id.value());
    }
}

Q_DECLARE_METATYPE(PMP::Client::LocalTrackId)

#endif
