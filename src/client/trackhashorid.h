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

#ifndef PMP_CLIENT_TRACKHASHORID_H
#define PMP_CLIENT_TRACKHASHORID_H

#include "common/filehash.h"
#include "common/nullable.h"

#include "trackserverid.h"

#include <QDebug>

#include <variant>

namespace PMP::Client
{
    /*! Contains either a track hash, a server-side track ID, or nothing. */
    class TrackHashOrId
    {
    public:
        TrackHashOrId()
         : _data{}
        {
            //
        }

        TrackHashOrId(FileHash hash)
         : _data(hash.isNull() ? datatype() : datatype(hash))
        {
            //
        }

        TrackHashOrId(TrackServerId id)
         : _data(id.isZero() ? datatype() : datatype(id))
        {
            //
        }

        constexpr bool isNull() const
        {
            return std::holds_alternative<std::monostate>(_data);
        }

        constexpr bool isHash() const
        {
            return std::holds_alternative<FileHash>(_data);
        }

        constexpr bool isId() const
        {
            return std::holds_alternative<TrackServerId>(_data);
        }

        Nullable<FileHash> toHash() const
        {
            if (std::holds_alternative<FileHash>(_data))
                return std::get<FileHash>(_data);

            return null;
        }

        Nullable<TrackServerId> toId() const
        {
            if (std::holds_alternative<TrackServerId>(_data))
                return std::get<TrackServerId>(_data);

            return null;
        }

        bool operator==(TrackHashOrId const& other) const
        {
            if (std::holds_alternative<std::monostate>(_data))
                return std::holds_alternative<std::monostate>(other._data);

            if (std::holds_alternative<FileHash>(_data))
            {
                return std::holds_alternative<FileHash>(other._data)
                       && std::get<FileHash>(_data) == std::get<FileHash>(other._data);
            }

            //if (std::holds_alternative<TrackServerId>(_data))
            {
                return std::holds_alternative<TrackServerId>(other._data)
                        && std::get<TrackServerId>(_data)
                              == std::get<TrackServerId>(other._data);
            }
        }

        bool operator==(FileHash const& other) const
        {
            return std::holds_alternative<FileHash>(_data)
                   && std::get<FileHash>(_data) == other;
        }

        bool operator==(TrackServerId const& otherId) const
        {
            return std::holds_alternative<TrackServerId>(_data)
                   && std::get<TrackServerId>(_data) == otherId;
        }

    private:
        typedef std::variant<std::monostate, FileHash, TrackServerId> datatype;
        datatype _data;
    };

    inline QDebug operator<<(QDebug debug, TrackHashOrId const& track)
    {
        if (track.isNull())
            debug << "(null)";
        else if (track.isHash())
            debug << track.toHash().value();
        else //if (track.isId())
            debug << "{SID" << track.toId().value() << "}";

        return debug;
    }
}
#endif
