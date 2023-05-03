/*
    Copyright (C) 2014-2022, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_TAGDATA_H
#define PMP_TAGDATA_H

#include <QMetaType>
#include <QString>

namespace PMP
{
    class TagData
    {
    public:
        TagData();
        TagData(const QString& artist, const QString& title,
                const QString& album, const QString& comment);

        QString artist() const { return _artist; }
        QString title() const { return _title; }
        QString album() const { return _album; }
        QString comment() const { return _comment; }

        bool operator == (TagData const& other) const
        {
            return _artist == other._artist
                && _title == other._title
                && _album == other._album
                && _comment == other._comment;
        }

        bool operator != (TagData const& other) const
        {
            return !(*this == other);
        }

    private:
        QString _artist;
        QString _title;
        QString _album;
        QString _comment;
    };
}

Q_DECLARE_METATYPE(PMP::TagData)

#endif
