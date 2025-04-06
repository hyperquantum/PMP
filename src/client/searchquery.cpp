/*
    Copyright (C) 2024-2025, Kevin André <hyperquantum@gmail.com>

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

#include "searchquery.h"

#include "common/searchutil.h"

namespace PMP::Client
{
    SearchQuery::SearchQuery()
    {
        //
    }

    SearchQuery::SearchQuery(const QString& query)
    {
        auto simplifiedSearchString = SearchUtil::toSearchString(query);

        _searchParts = simplifiedSearchString.split(QChar(' '), Qt::SkipEmptyParts);
    }

    void SearchQuery::clear()
    {
        _searchParts.clear();
    }

    bool SearchQuery::isEmpty() const
    {
        return _searchParts.isEmpty();
    }
}
