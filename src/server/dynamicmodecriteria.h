/*
    Copyright (C) 2020-2022, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_DYNAMICMODECRITERIA_H
#define PMP_DYNAMICMODECRITERIA_H

#include <QtGlobal>

namespace PMP::Server
{
    class DynamicModeCriteria
    {
    public:
        DynamicModeCriteria();

        void setNoRepetitionSpanSeconds(int seconds) { _noRepetitionSpan = seconds; }
        int noRepetitionSpanSeconds() const { return _noRepetitionSpan; }

        void setUser(quint32 user) { _user = user; }
        quint32 user() const { return _user; }

    private:
        int _noRepetitionSpan;
        quint32 _user;
    };

    inline bool operator==(DynamicModeCriteria const& first,
                           DynamicModeCriteria const& second)
    {
        return first.user() == second.user()
            && first.noRepetitionSpanSeconds() == second.noRepetitionSpanSeconds();
    }

    inline bool operator!=(DynamicModeCriteria const& first,
                           DynamicModeCriteria const& second)
    {
        return !(first == second);
    }
}
#endif
