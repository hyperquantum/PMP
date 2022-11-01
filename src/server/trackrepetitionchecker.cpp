/*
    Copyright (C) 2020-2022, Kevin Andre <hyperquantum@gmail.com>

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

#include "trackrepetitionchecker.h"

#include "history.h"
#include "playerqueue.h"
#include "queueentry.h"

namespace PMP
{
    TrackRepetitionChecker::TrackRepetitionChecker(QObject* parent, PlayerQueue* queue,
                                                   History* history)
     : QObject(parent),
       _currentTrack(nullptr),
       _queue(queue),
       _history(history),
       _noRepetitionSpanSeconds(0),
       _userPlayingFor(0)
    {
        //
    }

    int TrackRepetitionChecker::noRepetitionSpanSeconds() const
    {
        return _noRepetitionSpanSeconds;
    }

    bool TrackRepetitionChecker::isRepetitionWhenQueued(uint id, const FileHash& hash,
                                                        qint64 extraMarginMilliseconds)
    {
        if (_noRepetitionSpanSeconds < 0)
            return false; // repetition check disabled

        // check occurrence in queue

        auto repetition =
                _queue->checkPotentialRepetitionByAdd(hash, _noRepetitionSpanSeconds,
                                                      extraMarginMilliseconds);

        if (repetition.isRepetition())
            return true;

        qint64 millisecondsCounted = repetition.millisecondsCounted();
        if (millisecondsCounted >= _noRepetitionSpanSeconds * qint64(1000))
            return false;

        // check occurrence in 'now playing'
        auto trackNowPlaying = _currentTrack;
        if (trackNowPlaying)
        {
            auto currentHash = trackNowPlaying->hash().value();
            if (hash == currentHash)
                return true;
        }

        // check last play time, taking the future queue position into account

        QDateTime maxLastPlay =
                QDateTime::currentDateTimeUtc().addMSecs(millisecondsCounted)
                                               .addSecs(-_noRepetitionSpanSeconds);

        QDateTime lastPlay = _history->lastPlayedGloballySinceStartup(hash);
        if (lastPlay.isValid() && lastPlay > maxLastPlay)
            return true;

        auto maybeUserStats = _history->getUserStats(id, _userPlayingFor);
        if (maybeUserStats.isNull())
            return true;

        lastPlay = maybeUserStats.value().lastHeard();
        if (lastPlay.isValid() && lastPlay > maxLastPlay)
            return true;

        return false;
    }

    void TrackRepetitionChecker::setUserGeneratingFor(quint32 user)
    {
        _userPlayingFor = user;
    }

    void TrackRepetitionChecker::setNoRepetitionSpanSeconds(int seconds)
    {
        if (_noRepetitionSpanSeconds == seconds)
            return;

        _noRepetitionSpanSeconds = seconds;

        Q_EMIT noRepetitionSpanSecondsChanged();
    }

    void TrackRepetitionChecker::currentTrackChanged(
                                                QSharedPointer<const QueueEntry> newTrack)
    {
        _currentTrack = newTrack;
    }
}
