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

#include "scrobblingbackend.h"

#include <QtDebug>

namespace PMP {

    ScrobblingBackend::ScrobblingBackend()
     : QObject(nullptr), _state(ScrobblingBackendState::NotInitialized)
    {
        //
    }

    ScrobblingBackend::~ScrobblingBackend() {
        qDebug() << "running ~ScrobblingBackend()";
    }

    void ScrobblingBackend::setState(ScrobblingBackendState newState) {
        if (_state == newState) return; /* no change */

        _state = newState;
        emit stateChanged(newState);
    }

    QDebug operator<<(QDebug debug, ScrobblingBackendState state) {
        switch (state) {
            case ScrobblingBackendState::NotInitialized:
                debug << "ScrobblingBackendState::NotInitialized";
                break;
            case ScrobblingBackendState::WaitingForAuthenticationResult:
                debug << "ScrobblingBackendState::WaitingForAuthenticationResult";
                break;
            case ScrobblingBackendState::WaitingForScrobbleResult:
                debug << "ScrobblingBackendState::WaitingForScrobbleResult";
                break;
            case ScrobblingBackendState::WaitingForUserCredentials:
                debug << "ScrobblingBackendState::WaitingForUserCredentials";
                break;
            case ScrobblingBackendState::ReadyForScrobbling:
                debug << "ScrobblingBackendState::ReadyForScrobbling";
                break;
            case ScrobblingBackendState::TemporarilyUnavailable:
                debug << "ScrobblingBackendState::TemporarilyUnavailable";
                break;
            case ScrobblingBackendState::InvalidUserCredentials:
                debug << "ScrobblingBackendState::InvalidUserCredentials";
                break;
            case ScrobblingBackendState::PermanentFatalError:
                debug << "ScrobblingBackendState::PermanentFatalError";
                break;
            /*default:
                debug << int(state);
                break;*/
        }

        return debug;
    }

    QDebug operator<<(QDebug debug, ScrobbleResult result) {
        switch (result) {
            case ScrobbleResult::Success:
                debug << "ScrobbleResult::Success";
                break;
            case ScrobbleResult::Ignored:
                debug << "ScrobbleResult::Ignored";
                break;
            case ScrobbleResult::Error:
                debug << "ScrobbleResult::Error";
                break;
        }

        return debug;
    }

}