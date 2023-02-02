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

#ifndef PMP_TRACKREPETITIONCHECKER_H
#define PMP_TRACKREPETITIONCHECKER_H

#include "common/filehash.h"

#include <QObject>

namespace PMP::Server
{
    class History;
    class PlayerQueue;
    class QueueEntry;

    class TrackRepetitionChecker : public QObject
    {
        Q_OBJECT
    public:
        TrackRepetitionChecker(QObject* parent, PlayerQueue* queue, History* history);

        int noRepetitionSpanSeconds() const;

        bool isRepetitionWhenQueued(uint id, FileHash const& hash,
                                    qint64 extraMarginMilliseconds = 0);

    public Q_SLOTS:
        void setUserGeneratingFor(quint32 user);
        void setNoRepetitionSpanSeconds(int seconds);

        void currentTrackChanged(QSharedPointer<QueueEntry const> newTrack);

    Q_SIGNALS:
        void noRepetitionSpanSecondsChanged();

    private:
        QSharedPointer<QueueEntry const> _currentTrack;
        PlayerQueue* _queue;
        History* _history;
        int _noRepetitionSpanSeconds;
        quint32 _userPlayingFor;
    };
}
#endif
