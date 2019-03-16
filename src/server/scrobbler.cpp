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

#include "scrobbler.h"

#include "scrobblingbackend.h"
#include "scrobblingdataprovider.h"

#include <QDebug>
#include <QTimer>

namespace PMP {

    Scrobbler::Scrobbler(QObject* parent, ScrobblingDataProvider* dataProvider,
                         ScrobblingBackend* backend)
     : QObject(parent), _dataProvider(dataProvider), _backend(backend),
        _timeoutTimer(new QTimer(this)),
        _backoffTimer(new QTimer(this)), _backoffMilliseconds(0)
    {
        // TODO: make sure the data provider is cleaned up
        _backend->setParent(this);

        connect(
            backend, &ScrobblingBackend::stateChanged,
            this, &Scrobbler::backendStateChanged
        );
        connect(
            backend, &ScrobblingBackend::gotScrobbleResult,
            this, &Scrobbler::gotScrobbleResult
        );
        connect(
            backend, &ScrobblingBackend::serviceTemporarilyUnavailable,
            this, &Scrobbler::serviceTemporarilyUnavailable
        );

        _timeoutTimer->setSingleShot(true);
        connect(
            _timeoutTimer, &QTimer::timeout, this, &Scrobbler::timeoutTimerTimedOut
        );

        _backoffTimer->setSingleShot(true);
        connect(
            _backoffTimer, &QTimer::timeout, this, &Scrobbler::backoffTimerTimedOut
        );

        checkIfWeHaveSomethingToDo();
    }

    void Scrobbler::wakeUp() {
        qDebug() << "Scrobbler::wakeUp() called";
        checkIfWeHaveSomethingToDo();
    }

    void Scrobbler::timeoutTimerTimedOut() {
        qDebug() << "timeout event triggered; backend state:" << _backend->state();

        /* if a track was being scrobbled, reinsert it at the front of the queue */
        reinsertPendingScrobbleAtFrontOfQueue();

        /* TODO : handle other kinds of timeouts */
    }

    void Scrobbler::backoffTimerTimedOut() {
        qDebug() << "backoff timer triggered";
        checkIfWeHaveSomethingToDo();
    }

    void Scrobbler::checkIfWeHaveSomethingToDo() {
        qDebug() << "running checkIfWeHaveSomethingToDo";
        if (_pendingScrobble) return;
        if (_backoffTimer->isActive()) return;

        if (_tracksToScrobble.empty()) {
            _tracksToScrobble.append(_dataProvider->getNextTracksToScrobble().toList());

            if (_tracksToScrobble.empty()) return;
        }

        qDebug() << "we have" << _tracksToScrobble.size() << "tracks to scrobble";
        qDebug() << "backend state:" << _backend->state();
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
        qDebug() << "sendNextScrobble; scrobble queue size:" << _tracksToScrobble.size();
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
        if (!_pendingScrobble) {
            qDebug() << " did not expect a scrobble result right now";
            return;
        }

        _timeoutTimer->stop();

        if (result == ScrobbleResult::Error) {
            reinsertPendingScrobbleAtFrontOfQueue();
            startBackoffTimer(_backend->getInitialBackoffMillisecondsForErrorReply());
            return;
        }

        _backoffMilliseconds = 0;

        auto trackPtr = _pendingScrobble;
        _pendingScrobble = nullptr;

        switch (result) {
            case ScrobbleResult::Success:
                trackPtr->scrobbledSuccessfully();
                break;
            case ScrobbleResult::Ignored:
                trackPtr->scrobbleIgnored();
                break;
            case ScrobbleResult::Error: /* already handled before the switch */
                break;
        }

        QTimer::singleShot(0, this, SLOT(wakeUp()));
    }

    void Scrobbler::backendStateChanged(ScrobblingBackendState newState) {
        qDebug() << "backend state changed to:" << newState;

        _timeoutTimer->stop();
        _backoffMilliseconds = 0;

        /* should we wait for something to change in the backend? */
        switch (newState) {
            case ScrobblingBackendState::WaitingForAuthenticationResult:
                _timeoutTimer->start(30000);
                return;
            case ScrobblingBackendState::WaitingForScrobbleResult:
                _timeoutTimer->start(30000);
                return;
            case ScrobblingBackendState::WaitingForUserCredentials:
                return; /* no waiting for timeout */
            default:
                checkIfWeHaveSomethingToDo();
                break;
        }
    }

    void Scrobbler::serviceTemporarilyUnavailable() {
        qDebug() << "serviceTemporarilyUnavailable() called";

        reinsertPendingScrobbleAtFrontOfQueue();

        startBackoffTimer(_backend->getInitialBackoffMillisecondsForUnavailability());
    }

    void Scrobbler::startBackoffTimer(int initialBackoffMilliseconds) {
        _backoffTimer->stop();

        if (_backoffMilliseconds < initialBackoffMilliseconds) {
            _backoffMilliseconds = initialBackoffMilliseconds;
        }
        else {
            _backoffMilliseconds = (_backoffMilliseconds + 10) * 2;
        }

        qDebug() << "starting backoff timer with interval:" << _backoffMilliseconds;
        _backoffTimer->start(_backoffMilliseconds);
    }

    void Scrobbler::reinsertPendingScrobbleAtFrontOfQueue() {
        auto trackPtr = _pendingScrobble;
        _pendingScrobble = nullptr;

        if (trackPtr) {
            /* reinsert at front of the queue */
            _tracksToScrobble.insert(0, trackPtr);
        }
    }

}
