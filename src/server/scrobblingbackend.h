/*
    Copyright (C) 2018-2024, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_SCROBBLINGBACKEND_H
#define PMP_SCROBBLINGBACKEND_H

#include "common/future.h"

#include "result.h"
#include "scrobblingtrack.h"

#include <QObject>
#include <QtDebug>

namespace PMP::Server
{
    enum class ScrobblingBackendState
    {
        NotInitialized = 0,
        WaitingForUserCredentials,
        ReadyForScrobbling,
        PermanentFatalError,
    };

    QDebug operator<<(QDebug debug, ScrobblingBackendState state);

    enum class ScrobbleResult
    {
        Success = 0,
        Ignored,
        Error
    };

    QDebug operator<<(QDebug debug, ScrobbleResult result);

    class ScrobblingBackend : public QObject
    {
        Q_OBJECT
    public:
        ScrobblingBackend();
        virtual ~ScrobblingBackend();

        ScrobblingBackendState state() const { return _state; }

        virtual void initialize();

        virtual void updateNowPlaying(ScrobblingTrack track) = 0;
        virtual void scrobbleTrack(QDateTime timestamp, ScrobblingTrack track) = 0;

        virtual SimpleFuture<Result> authenticateWithCredentials(QString usernameOrEmail,
                                                                 QString password) = 0;

        int getDelayInMillisecondsBetweenSubsequentScrobbles() const
        {
            return _delayInMillisecondsBetweenSubsequentScrobbles;
        }

        int getInitialBackoffMillisecondsForUnavailability() const
        {
            return _initialBackoffMillisecondsForUnavailability;
        }

        int getInitialBackoffMillisecondsForErrorReply() const
        {
            return _initialBackoffMillisecondsForErrorReply;
        }

    Q_SIGNALS:
        void stateChanged(PMP::Server::ScrobblingBackendState newState,
                          PMP::Server::ScrobblingBackendState oldState);
        void gotNowPlayingResult(bool success);
        void gotScrobbleResult(PMP::Server::ScrobbleResult result);
        void serviceTemporarilyUnavailable();

    protected Q_SLOTS:
        void setState(PMP::Server::ScrobblingBackendState newState);
        void setDelayInMillisecondsBetweenSubsequentScrobbles(int timeMilliseconds);
        void setInitialBackoffMillisecondsForUnavailability(int timeMilliseconds);
        void setInitialBackoffMillisecondsForErrorReply(int timeMilliseconds);

    protected:
        virtual bool needsSsl() const = 0;

    private:
        int _delayInMillisecondsBetweenSubsequentScrobbles;
        int _initialBackoffMillisecondsForUnavailability;
        int _initialBackoffMillisecondsForErrorReply;
        ScrobblingBackendState _state;
    };
}

Q_DECLARE_METATYPE(PMP::Server::ScrobblingBackendState)
Q_DECLARE_METATYPE(PMP::Server::ScrobbleResult)

#endif
