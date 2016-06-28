/*
    Copyright (C) 2014-2016, Kevin Andre <hyperquantum@gmail.com>

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

#include "common/filehash.h"

#include <QDateTime>
#include <QHash>
#include <QObject>

namespace PMP {

    class FileHash;
    class Player;
    class QueueEntry;

    class History : public QObject {
        Q_OBJECT
    public:
        History(Player* player);

        QDateTime lastPlayed(FileHash const& hash) const;

    Q_SIGNALS:
        void updatedHashUserStats(uint hashID, quint32 user,
                                  QDateTime previouslyHeard, qint16 score);

    private slots:
        void currentTrackChanged(QueueEntry const* newTrack);
        void failedToPlayTrack(QueueEntry const* track);
        void donePlayingTrack(QueueEntry const* track, int permillage, bool hadError,
                              bool hadSeek);

    private:
        Player* _player;
        QHash<FileHash, QDateTime> _lastPlayHash;
        QueueEntry const* _nowPlaying;
    };

}
#endif
