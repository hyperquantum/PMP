/*
    Copyright (C) 2020-2023, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_CLIENT_CURRENTTRACKMONITOR_H
#define PMP_CLIENT_CURRENTTRACKMONITOR_H

#include "common/tribool.h"
#include "common/playerstate.h"

#include "localhashid.h"

#include <QObject>

namespace PMP::Client
{
    class CurrentTrackMonitor : public QObject
    {
        Q_OBJECT
    public:
        virtual ~CurrentTrackMonitor() {}

        virtual PlayerState playerState() const = 0;

        virtual TriBool isTrackPresent() const = 0;
        virtual quint32 currentQueueId() const = 0;
        virtual qint64 currentTrackProgressMilliseconds() const = 0;

        virtual LocalHashId currentTrackHash() const = 0;

        virtual QString currentTrackTitle() const = 0;
        virtual QString currentTrackArtist() const = 0;
        virtual QString currentTrackPossibleFilename() const = 0;
        virtual qint64 currentTrackLengthMilliseconds() const = 0;

    public Q_SLOTS:
        virtual void seekTo(qint64 positionInMilliseconds) = 0;

    Q_SIGNALS:
        void currentTrackChanged();
        void currentTrackInfoChanged();
        void trackProgressChanged(PlayerState state, quint32 queueId,
                                  qint64 progressInMilliseconds,
                                  qint64 trackLengthInMilliseconds);

    protected:
        explicit CurrentTrackMonitor(QObject* parent = nullptr) : QObject(parent) {}
    };
}
#endif
