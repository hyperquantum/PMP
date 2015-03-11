/*
    Copyright (C) 2014-2015, Kevin Andre <hyperquantum@gmail.com>

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

#include "history.h"

#include "player.h"
#include "queueentry.h"

namespace PMP {

    History::History(Player* player)
     : _player(player), _nowPlaying(0)
    {
        connect(player, SIGNAL(currentTrackChanged(QueueEntry const*)), this, SLOT(currentTrackChanged(QueueEntry const*)));
    }

    QDateTime History::lastPlayed(HashID const& hash) const {
        return _lastPlayHash[hash];
    }

    void History::currentTrackChanged(QueueEntry const* newTrack) {
        if (_nowPlaying != 0 && newTrack != _nowPlaying) {
            const HashID* hash = _nowPlaying->hash();
            // TODO: make sure hash is known, so history won't get lost
            if (hash != 0) {
                _lastPlayHash[*hash] = QDateTime::currentDateTimeUtc();
            }
        }

        _nowPlaying = newTrack;
    }

}
