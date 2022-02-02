/*
    Copyright (C) 2017-2022, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_PLAYERCONTROLLER_H
#define PMP_PLAYERCONTROLLER_H

#include "common/tribool.h"

#include "playermode.h"
#include "playerstate.h"
#include "requestid.h"
#include "resultmessageerrorcode.h"

#include <QDateTime>
#include <QObject>
#include <QString>

namespace PMP
{
    class PlayerController : public QObject
    {
        Q_OBJECT
    public:
        virtual ~PlayerController() {}

        virtual PlayerState playerState() const = 0;
        virtual TriBool delayedStartActive() const = 0;
        virtual TriBool isTrackPresent() const = 0;
        virtual quint32 currentQueueId() const = 0;
        virtual uint queueLength() const = 0;
        virtual bool canPlay() const = 0;
        virtual bool canPause() const = 0;
        virtual bool canSkip() const = 0;

        virtual PlayerMode playerMode() const = 0;
        virtual quint32 personalModeUserId() const = 0;
        virtual QString personalModeUserLogin() const = 0;

        virtual int volume() const = 0;

        virtual RequestID activateDelayedStart(qint64 delayMilliseconds) = 0;
        virtual RequestID activateDelayedStart(QDateTime startTime) = 0;
        virtual RequestID deactivateDelayedStart() = 0;

    public Q_SLOTS:
        virtual void play() = 0;
        virtual void pause() = 0;
        virtual void skip() = 0;

        virtual void setVolume(int volume) = 0;

        virtual void switchToPublicMode() = 0;
        virtual void switchToPersonalMode() = 0;

    Q_SIGNALS:
        void playerStateChanged(PlayerState playerState);
        void currentTrackChanged();
        void playerModeChanged(PlayerMode playerMode, quint32 personalModeUserId,
                               QString personalModeUserLogin);
        void volumeChanged();
        void queueLengthChanged();
        void delayedStartActiveChanged();

        void delayedStartActivationResultEvent(ResultMessageErrorCode errorCode,
                                               RequestID requestId);
        void delayedStartDeactivationResultEvent(ResultMessageErrorCode errorCode,
                                                 RequestID requestId);

    protected:
        explicit PlayerController(QObject* parent) : QObject(parent) {}
    };
}
#endif
