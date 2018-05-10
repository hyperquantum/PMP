/*
    Copyright (C) 2018, Kevin Andre <hyperquantum@gmail.com>

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
        WaitingForAuthenticationResult,
        ReadyForScrobbling,
        WaitingForScrobbleResult,
        TemporarilyUnavailable,
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

        virtual void scrobbleTrack(QDateTime timestamp, QString const& title,
                                   QString const& artist, QString const& album,
                                   int trackDurationSeconds = -1) = 0;

    public slots:
        virtual void initialize() = 0;

    Q_SIGNALS:
        void stateChanged(ScrobblingBackendState newState);
        void gotScrobbleResult(ScrobbleResult result);

    protected slots:
        void setState(ScrobblingBackendState newState);

    private:
        ScrobblingBackendState _state;
    };
}

Q_DECLARE_METATYPE(PMP::ScrobblingBackendState)
Q_DECLARE_METATYPE(PMP::ScrobbleResult)

#endif
