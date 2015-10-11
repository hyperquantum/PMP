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

#include "database.h"
#include "player.h"
#include "queueentry.h"
#include "resolver.h"

#include <QRunnable>
#include <QThreadPool>

namespace PMP {

    class AddToHistoryTask : public QRunnable {
    public:
        AddToHistoryTask(uint hashID, quint32 user, QDateTime started, QDateTime ended,
                         int permillage, bool validForScoring);

        void run();

    private:
        uint _hashID;
        quint32 _user;
        QDateTime _started;
        QDateTime _ended;
        int _permillage;
        bool _validForScoring;
    };

    AddToHistoryTask::AddToHistoryTask(uint hashID, quint32 user, QDateTime started,
                                       QDateTime ended, int permillage,
                                       bool validForScoring)
     : _hashID(hashID), _user(user), _started(started), _ended(ended),
       _permillage(permillage), _validForScoring(validForScoring)
    {
        //
    }

    void AddToHistoryTask::run() {
        auto db = Database::getDatabaseForCurrentThread();

        if (db != nullptr) {
            db->addToHistory(
                _hashID, _user, _started, _ended, _permillage, _validForScoring
            );
        }
    }

    History::History(Player* player)
     : _player(player), _nowPlaying(0)
    {
        connect(
            player, &Player::currentTrackChanged,
            this, &History::currentTrackChanged
        );

        connect(
            player, &Player::failedToPlayTrack,
            this, &History::failedToPlayTrack
        );

        connect(
            player, &Player::donePlayingTrack,
            this, &History::donePlayingTrack
        );
    }

    QDateTime History::lastPlayed(FileHash const& hash) const {
        return _lastPlayHash[hash];
    }

    void History::currentTrackChanged(QueueEntry const* newTrack) {
        if (_nowPlaying != 0 && newTrack != _nowPlaying) {
            const FileHash* hash = _nowPlaying->hash();
            /* TODO: make sure hash is known, so history won't get lost */
            if (hash != 0) {
                _lastPlayHash[*hash] = QDateTime::currentDateTimeUtc();
            }
        }

        _nowPlaying = newTrack;
    }

    void History::failedToPlayTrack(const QueueEntry *track) {
        /* we don't use this yet */
    }

    void History::donePlayingTrack(QueueEntry const* track, int permillage, bool hadError,
                                   bool hadSeek)
    {
        const FileHash* hash = track->hash();
        uint hashID = _player->resolver().getID(*hash);
        quint32 user = _player->userPlayingFor();

        auto task =
            new AddToHistoryTask(
                hashID, user, track->started(), track->ended(), permillage,
                !(hadError || hadSeek)
            );

        QThreadPool::globalInstance()->start(task);
    }

}
