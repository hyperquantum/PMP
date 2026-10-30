/*
    Copyright (C) 2025-2026, Kevin André <hyperquantum@gmail.com>

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

#ifndef PMP_CLIENT_TRACKSERVERID_H
#define PMP_CLIENT_TRACKSERVERID_H

#include <QDebug>
#include <QMetaType>
#include <QtGlobal>
//#include <QtTypes> <<-- for quint64

namespace PMP::Client
{
    /*! Contains a server-side track ID, or nothing (zero). */
    class TrackServerId
    {
    public:
        constexpr TrackServerId() : _id(0) {}
        constexpr explicit TrackServerId(quint64 id) : _id(id) {}

        constexpr quint64 value() const { return _id; }
        constexpr bool hasValue() const { return _id > 0; }
        constexpr bool isZero() const { return _id == 0; }

        TrackServerId& operator=(TrackServerId const&) = default;

        constexpr bool operator==(TrackServerId const& other) const
        {
            return _id == other._id;
        }

    private:
        quint64 _id;
    };

    inline constexpr bool operator<(TrackServerId id1, TrackServerId id2)
    {
        return id1.value() < id2.value();
    }

    inline QDebug operator<<(QDebug debug, const TrackServerId id)
    {
        debug << id.value();
        return debug;
    }

    constexpr inline size_t qHash(const TrackServerId& id, size_t seed)
    {
        return qHashMulti(seed, id.value());
    }
}

Q_DECLARE_METATYPE(PMP::Client::TrackServerId)

#endif
