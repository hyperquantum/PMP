/*
    Copyright (C) 2025, Kevin André <hyperquantum@gmail.com>

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

#ifndef PMP_SEARCHRANK_H
#define PMP_SEARCHRANK_H

#include "searchquery.h"

namespace PMP::Client
{
    class SearchRank
    {
    public:
        static bool isBetterMatchThan(SearchQuery const& query, QString s1, QString s2);
        static int compareMatches(SearchQuery const& query, QString s1, QString s2);
        static int getMatchScore(SearchQuery const& query, QString s);

        static bool isMatch(SearchQuery const& query, QString s)
        {
            return getMatchScore(query, s) > 0;
        };
    };
}
#endif
