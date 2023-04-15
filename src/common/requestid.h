/*
    Copyright (C) 2021-2023, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_REQUESTID_H
#define PMP_REQUESTID_H

#include <QtGlobal>

namespace PMP
{
    class RequestID
    {
    public:
        constexpr RequestID() : _rawId(0) {}
        constexpr explicit RequestID(uint rawId) : _rawId(rawId) {}

        constexpr bool isValid() const { return _rawId > 0; }
        constexpr uint rawId() const { return _rawId; }

    private:
        uint _rawId;
    };

    constexpr inline bool operator==(const RequestID& me, const RequestID& other)
    {
        return me.rawId() == other.rawId();
    }

    constexpr inline bool operator!=(const RequestID& me, const RequestID& other)
    {
        return !(me == other);
    }

    constexpr inline uint qHash(const RequestID& requestId)
    {
        return requestId.rawId();
    }
}
#endif
