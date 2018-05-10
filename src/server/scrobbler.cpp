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

#include "scrobbler.h"

#include "scrobblingbackend.h"

#include <QDebug>
#include <QTimer>

namespace PMP {

    TrackToScrobble::TrackToScrobble() {
        //
    }

    TrackToScrobble::~TrackToScrobble() {
        //
    }

    /* ======================================================================= */

    ScrobblingDataProvider::ScrobblingDataProvider() {
        //
    }

    ScrobblingDataProvider::~ScrobblingDataProvider() {
        //
    }

    /* ======================================================================= */

    Scrobbler::Scrobbler(QObject* parent, ScrobblingDataProvider* dataProvider,
                         ScrobblingBackend* backend)
     : QObject(parent), _dataProvider(dataProvider), _backend(backend),
        _timeoutTimer(new QTimer(this))
    {
        _backend->setParent(this);

        connect(
            backend, &ScrobblingBackend::stateChanged,
            this, &Scrobbler::backendStateChanged
        );
        connect(
            backend, &ScrobblingBackend::gotScrobbleResult,
            this, &Scrobbler::gotScrobbleResult
        );

        _timeoutTimer->setSingleShot(true);
        connect(
            _timeoutTimer, &QTimer::timeout, this, &Scrobbler::timeoutTimerTimedOut
        );

        checkIfWeHaveSomethingToDo();
    }

    void Scrobbler::wakeUp() {
        checkIfWeHaveSomethingToDo();
    }

    void Scrobbler::timeoutTimerTimedOut() {
        qDebug() << "timeout event triggered; backend state:" << _backend->state();

        /* TODO */
    }

    void Scrobbler::checkIfWeHaveSomethingToDo() {
        qDebug() << "running checkIfWeHaveSomethingToDo";
        if (_pendingScrobble) return;

        if (_tracksToScrobble.empty()) {
            _tracksToScrobble.append(_dataProvider->getNextTracksToScrobble().toList());

            if (_tracksToScrobble.empty()) return;
        }

        qDebug() << " backend state:" << _backend->state();
        switch (_backend->state()) {
            case ScrobblingBackendState::NotInitialized:
                _backend->initialize();
                QTimer::singleShot(0, this, SLOT(wakeUp()));
                break;
            case ScrobblingBackendState::WaitingForAuthenticationResult:
            case ScrobblingBackendState::WaitingForScrobbleResult:
                break; /* waiting for timeout event */
            case ScrobblingBackendState::ReadyForScrobbling:
                sendNextScrobble();
                break;
            case ScrobblingBackendState::TemporarilyUnavailable:
                /* TODO */
                break;
            case ScrobblingBackendState::PermanentFatalError:
                /* TODO */
                break;
            case ScrobblingBackendState::WaitingForUserCredentials:
            case ScrobblingBackendState::InvalidUserCredentials:
                /* we will have to wait for (new) credentials; this means waiting until
                   the state of the backend changes again*/
                break;
        }
    }

    void Scrobbler::sendNextScrobble() {
        qDebug() << "sendNextScrobble; queue size:" << _tracksToScrobble.size();
        if (_tracksToScrobble.empty() || _pendingScrobble) return;

        auto trackPtr = _tracksToScrobble.dequeue();
        _pendingScrobble = trackPtr;

        auto& track = *trackPtr;

        _timeoutTimer->stop();
        _timeoutTimer->start(5000);

        _backend->scrobbleTrack(
            track.timestamp(), track.title(), track.artist(), track.album()
        );
        /* then we wait for the gotScrobbleResult event to arrive */
    }

    void Scrobbler::gotScrobbleResult(ScrobbleResult result) {
        qDebug() << "received scrobble result:" << result;
        if (!_pendingScrobble) return;

        _timeoutTimer->stop();
        auto trackPtr = _pendingScrobble;
        _pendingScrobble = nullptr;

        switch (result) {
            case ScrobbleResult::Success:
                emit trackPtr->scrobbledSuccessfully();
                break;
            case ScrobbleResult::Ignored:
                emit trackPtr->cannotBeScrobbled();
                break;
            case ScrobbleResult::Error:
                /* reinsert at front of the queue */
                _tracksToScrobble.insert(0, trackPtr);
                break;
        }

        QTimer::singleShot(0, this, SLOT(wakeUp()));
    }

    void Scrobbler::backendStateChanged(ScrobblingBackendState newState) {
        qDebug() << "backend state changed to:" << newState;

        _timeoutTimer->stop();

        /* should we wait for something to change in the backend? */
        switch (newState) {
            case ScrobblingBackendState::WaitingForAuthenticationResult:
                _timeoutTimer->start(5000);
                break;
            case ScrobblingBackendState::WaitingForScrobbleResult:
                _timeoutTimer->start(5000);
                break;
            case ScrobblingBackendState::WaitingForUserCredentials:
                break; /* no waiting for timeout */
            default:
                break; /* no waiting for timeout */
        }

        checkIfWeHaveSomethingToDo();
    }

}
