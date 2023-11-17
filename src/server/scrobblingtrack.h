/*
    Copyright (C) 2023, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_SERVER_SCROBBLINGTRACK_H
#define PMP_SERVER_SCROBBLINGTRACK_H

#include <QMetaType>
#include <QString>

namespace PMP::Server
{
    class ScrobblingTrack
    {
    public:
        ScrobblingTrack() {}

        ScrobblingTrack(QString const& title, QString const& artist)
            : title(title), artist(artist)
        {
            //
        }

        ScrobblingTrack(QString const& title, QString const& artist,
                        QString const& album, QString const& albumArtist)
            : title(title), artist(artist), album(album), albumArtist(albumArtist)
        {
            //
        }

        void clear()
        {
            title.clear();
            artist.clear();
            album.clear();
            albumArtist.clear();
            durationInSeconds = -1;
        }

        QString title, artist, album, albumArtist;
        int durationInSeconds { -1 };
    };
}

Q_DECLARE_METATYPE(PMP::Server::ScrobblingTrack)

#endif
