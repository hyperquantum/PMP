/*
    Copyright (C) 2015-2023, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_SERVER_COLLECTIONTRACKINFO_H
#define PMP_SERVER_COLLECTIONTRACKINFO_H

#include "common/filehash.h"

#include <QMetaType>
#include <QString>

namespace PMP::Server
{
    class CollectionTrackInfo
    {
    public:
        CollectionTrackInfo()
         : _isAvailable(false), _lengthInMs(-1)
        {
            //
        }

        CollectionTrackInfo(FileHash const& hash, bool isAvailable)
         : _hash(hash), _isAvailable(isAvailable), _lengthInMs(-1)
        {
            //
        }

        CollectionTrackInfo(FileHash const& hash, bool isAvailable,
                            QString const& title, QString const& artist,
                            QString const& album, QString const& albumArtist,
                            qint32 lengthInMilliseconds)
         : _hash(hash), _isAvailable(isAvailable), _lengthInMs(lengthInMilliseconds),
            _title(title), _artist(artist), _album(album), _albumArtist(albumArtist)
        {
            //
        }

        const FileHash& hash() const { return _hash; }

        void setAvailable(bool available) { _isAvailable = available; }
        bool isAvailable() const { return _isAvailable; }

        const QString& title() const { return _title; }
        const QString& artist() const { return _artist; }
        const QString& album() const { return _album; }
        const QString& albumArtist() const { return _albumArtist; }
        bool lengthIsKnown() const { return _lengthInMs >= 0; }
        qint32 lengthInMilliseconds() const { return _lengthInMs; }

        qint32 lengthInSeconds() const
        {
            return lengthIsKnown() ? _lengthInMs / 1000 : -1;
        }

        bool titleAndArtistUnknown() const
        {
            return _title.isEmpty() && _artist.isEmpty();
        }

    private:
        FileHash _hash;
        bool _isAvailable;
        qint32 _lengthInMs;
        QString _title, _artist, _album, _albumArtist;
    };

    inline bool operator==(const CollectionTrackInfo& me,
                           const CollectionTrackInfo& other)
    {
        return me.hash() == other.hash()
            && me.isAvailable() == other.isAvailable()
            && me.lengthInMilliseconds() == other.lengthInMilliseconds()
            && me.title() == other.title()
            && me.artist() == other.artist()
            && me.album() == other.album()
            && me.albumArtist() == other.albumArtist();
    }

    inline bool operator!=(const CollectionTrackInfo& me,
                           const CollectionTrackInfo& other)
    {
        return !(me == other);
    }
}

Q_DECLARE_METATYPE(PMP::Server::CollectionTrackInfo)

#endif
