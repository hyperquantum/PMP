/*
    Copyright (C) 2015-2016, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_COLLECTIONTRACKINFO_H
#define PMP_COLLECTIONTRACKINFO_H

#include "filehash.h"

#include <QMetaType>
#include <QString>

namespace PMP {

    class CollectionTrackInfo {
    public:
        CollectionTrackInfo()
         : _isAvailable(false)
        {
            //
        }

        CollectionTrackInfo(FileHash const& hash, bool isAvailable,
                            QString const& title, QString const& artist)
        : _hash(hash), _isAvailable(isAvailable), _title(title), _artist(artist)
        {
            //
        }

        const FileHash& hash() const { return _hash; }
        bool isAvailable() const { return _isAvailable; }
        const QString& title() const { return _title; }
        const QString& artist() const { return _artist; }
        // TODO: track length

        bool titleAndArtistUnknown() const {
            return _title.isEmpty() && _artist.isEmpty();
        }

    private:
        FileHash _hash;
        bool _isAvailable;
        QString _title, _artist;
    };

}

Q_DECLARE_METATYPE(PMP::CollectionTrackInfo)

#endif

