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

#ifndef PMP_HISTORY_H
#define PMP_HISTORY_H

#include "common/hashid.h"

#include <QDateTime>
#include <QHash>
#include <QObject>

namespace PMP {

    class HashID;
    class Player;
    class QueueEntry;

    class History : public QObject {
        Q_OBJECT
    public:
        History(Player* player);

        QDateTime lastPlayed(HashID const& hash) const;

    private slots:
        void currentTrackChanged(QueueEntry const* newTrack);

    private:
        Player* _player;
        QHash<HashID, QDateTime> _lastPlayHash;
        QueueEntry const* _nowPlaying;
    };

}
#endif
