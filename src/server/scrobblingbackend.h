/*
    Copyright (C) 2018-2019, Kevin Andre <hyperquantum@gmail.com>

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

#include <QObject>
#include <QtDebug>

namespace PMP {

    enum class ScrobblingBackendState {
        NotInitialized = 0,
        WaitingForUserCredentials,
        ReadyForScrobbling,
        InvalidUserCredentials,
        PermanentFatalError,
    };

    QDebug operator<<(QDebug debug, ScrobblingBackendState state);

    enum class ScrobbleResult {
        Success = 0,
        Ignored,
        Error
    };

    QDebug operator<<(QDebug debug, ScrobbleResult result);

    class ScrobblingBackend : public QObject {
        Q_OBJECT
    public:
        ScrobblingBackend();
        virtual ~ScrobblingBackend();

        ScrobblingBackendState state() const { return _state; }
        bool waitingForReply() const { return _waitingForReply; }

        virtual void scrobbleTrack(QDateTime timestamp, QString const& title,
                                   QString const& artist, QString const& album,
                                   int trackDurationSeconds = -1) = 0;

        int getDelayInMillisecondsBetweenSubsequentScrobbles() const {
            return _delayInMillisecondsBetweenSubsequentScrobbles;
        }

        int getInitialBackoffMillisecondsForUnavailability() const {
            return _initialBackoffMillisecondsForUnavailability;
        }

        int getInitialBackoffMillisecondsForErrorReply() const {
            return _initialBackoffMillisecondsForErrorReply;
        }

    public slots:
        virtual void initialize() = 0;

    Q_SIGNALS:
        void stateChanged(ScrobblingBackendState newState,
                                                         ScrobblingBackendState oldState);
        void gotScrobbleResult(ScrobbleResult result);
        void serviceTemporarilyUnavailable();

    protected slots:
        void setState(ScrobblingBackendState newState);
        void setWaitingForReply(bool waiting);
        void setDelayInMillisecondsBetweenSubsequentScrobbles(int timeMilliseconds);
        void setInitialBackoffMillisecondsForUnavailability(int timeMilliseconds);
        void setInitialBackoffMillisecondsForErrorReply(int timeMilliseconds);

    private:
        int _delayInMillisecondsBetweenSubsequentScrobbles;
        int _initialBackoffMillisecondsForUnavailability;
        int _initialBackoffMillisecondsForErrorReply;
        ScrobblingBackendState _state;
        bool _waitingForReply;
    };
}

Q_DECLARE_METATYPE(PMP::ScrobblingBackendState)
Q_DECLARE_METATYPE(PMP::ScrobbleResult)

#endif
