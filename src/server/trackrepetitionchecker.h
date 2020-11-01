/*
    Copyright (C) 2020, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_TRACKREPETITIONCHECKER_H
#define PMP_TRACKREPETITIONCHECKER_H

#include "common/filehash.h"

#include <QObject>

namespace PMP {

    class History;
    class PlayerQueue;
    class QueueEntry;

    class TrackRepetitionChecker : public QObject {
        Q_OBJECT
    public:
        TrackRepetitionChecker(QObject* parent, PlayerQueue* queue, History* history);

        int noRepetitionSpanSeconds() const;

        bool isRepetitionWhenQueued(uint id, FileHash const& hash);

    public Q_SLOTS:
        void setUserGeneratingFor(quint32 user);
        void setNoRepetitionSpanSeconds(int seconds);

        void currentTrackChanged(QueueEntry const* newTrack);

    Q_SIGNALS:
        void noRepetitionSpanSecondsChanged();

    private:
        QueueEntry const* _currentTrack;
        PlayerQueue* _queue;
        History* _history;
        int _noRepetitionSpan;
        quint32 _userPlayingFor;
    };
}
#endif
