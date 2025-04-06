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

#ifndef PMP_SEARCHQUERY_H
#define PMP_SEARCHQUERY_H

#include <QString>
#include <QStringList>

namespace PMP
{
    class SearchData;
}

namespace PMP::Client
{
    class SearchRank;

    class SearchQuery
    {
    public:
        SearchQuery();
        SearchQuery(QString const& query);

        void clear();
        bool isEmpty() const;

    private:
        friend class ::PMP::SearchData;
        friend class SearchRank;

        QStringList _searchParts;
    };
}
#endif
