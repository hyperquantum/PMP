/*
    Copyright (C) 2018-2022, Kevin Andre <hyperquantum@gmail.com>

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

namespace PMP::Server
{
    Scrobbler::Scrobbler(QObject* parent, ScrobblingDataProvider* dataProvider,
                         ScrobblingBackend* backend)
     : QObject(parent), _dataProvider(dataProvider), _backend(backend),
        _status(ScrobblerStatus::Unknown),
        _timeoutTimer(new QTimer(this)),
        _backoffTimer(new QTimer(this)), _backoffMilliseconds(0),
        _nowPlayingTrackDurationSeconds(-1), _nowPlayingPresent(false),
        _nowPlayingSent(false), _nowPlayingDone(false)
    {
        // TODO: make sure the data provider is cleaned up
        _backend->setParent(this);

        connect(
            backend, &ScrobblingBackend::stateChanged,
            this, &Scrobbler::backendStateChanged
        );
        connect(
            backend, &ScrobblingBackend::gotNowPlayingResult,
            this, &Scrobbler::gotNowPlayingResult
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

        /* now wait for someone to call the wakeUp slot before doing anything */
    }

    void Scrobbler::wakeUp()
    {
        qDebug() << "Scrobbler::wakeUp() called";
        checkIfWeHaveSomethingToDo();
    }

    void Scrobbler::nowPlayingNothing()
    {
        qDebug() << "Scrobbler::nowPlayingNothing() called";

        _nowPlayingPresent = false;
        //_nowPlayingStartTime =
        _nowPlayingTitle.clear();
        _nowPlayingArtist.clear();
        _nowPlayingAlbum.clear();
        _nowPlayingTrackDurationSeconds = -1;
    }

    void Scrobbler::nowPlayingTrack(QDateTime startTime,
                                    QString const& title, QString const& artist,
                                    QString const& album, int trackDurationSeconds)
    {
        qDebug() << "Scrobbler::nowPlayingTrack() called";

        if (_nowPlayingPresent && _nowPlayingStartTime == startTime)
            return; /* still the same */

        if (title.isEmpty() || artist.isEmpty())
        {
            qDebug() << "cannot update 'now playing' because title or artist is missing";
            nowPlayingNothing();
            return;
        }

        _nowPlayingPresent = true;
        _nowPlayingSent = false;
        _nowPlayingDone = false;
        _nowPlayingStartTime = startTime;
        _nowPlayingTitle = title;
        _nowPlayingArtist = artist;
        _nowPlayingAlbum = album;
        _nowPlayingTrackDurationSeconds = trackDurationSeconds;

        checkIfWeHaveSomethingToDo();
    }

    void Scrobbler::timeoutTimerTimedOut()
    {
        qDebug() << "timeout event triggered; backend state:" << _backend->state();

        /* if a track was being scrobbled, reinsert it at the front of the queue */
        reinsertPendingScrobbleAtFrontOfQueue();

        /* TODO : handle other kinds of timeouts */
    }

    void Scrobbler::backoffTimerTimedOut()
    {
        qDebug() << "backoff timer triggered";
        checkIfWeHaveSomethingToDo();
    }

    void Scrobbler::checkIfWeHaveSomethingToDo()
    {
        qDebug() << "running checkIfWeHaveSomethingToDo";
        if (_nowPlayingSent && !_nowPlayingDone) return;
        if (_pendingScrobble) return;
        if (_backoffTimer->isActive()) return;

        if (_tracksToScrobble.empty())
        {
            _tracksToScrobble.append(_dataProvider->getNextTracksToScrobble().toList());
        }

        auto haveTracksToScrobble = !_tracksToScrobble.empty();
        auto haveNowPlayingToSend = _nowPlayingPresent && !_nowPlayingSent;

        if (!haveTracksToScrobble && !haveNowPlayingToSend)
            return; /* nothing to be done */

        qDebug() << "we have" << _tracksToScrobble.size() << "tracks to scrobble";

        if (haveNowPlayingToSend)
            qDebug() << "we have a 'now playing' to send";

        qDebug() << "backend state:" << _backend->state();
        switch (_backend->state())
        {
            case ScrobblingBackendState::NotInitialized:
                _backend->initialize();
                QTimer::singleShot(0, this, &Scrobbler::wakeUp);
                break;
            case ScrobblingBackendState::ReadyForScrobbling:
                if (haveTracksToScrobble)
                    sendNextScrobble();
                else if (haveNowPlayingToSend)
                    sendNowPlaying();
                break;
            case ScrobblingBackendState::PermanentFatalError:
                /* TODO */
                break;
            case ScrobblingBackendState::WaitingForUserCredentials:
                /* we will have to wait for (new) credentials; this means waiting until
                   the state of the backend changes again*/
                break;
        }
    }

    void Scrobbler::sendNowPlaying()
    {
        if (!_nowPlayingPresent || _nowPlayingSent) return;

        _nowPlayingSent = true;
        _nowPlayingDone = false;

        _timeoutTimer->stop();
        _timeoutTimer->start(5000);

        _backend->updateNowPlaying(
            _nowPlayingTitle, _nowPlayingArtist, _nowPlayingAlbum,
            _nowPlayingTrackDurationSeconds
        );
        /* then we wait for the gotNowPlayingResult event to arrive */
    }

    void Scrobbler::sendNextScrobble()
    {
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

    void Scrobbler::gotNowPlayingResult(bool success)
    {
        qDebug() << "received 'now playing' result:" << (success ? "success" : "failure");

        _timeoutTimer->stop();

        if (!success)
        {
            _nowPlayingSent = false;
            startBackoffTimer(_backend->getInitialBackoffMillisecondsForErrorReply());
            return;
        }

        _nowPlayingDone = true;

        reevaluateStatus(); /* status may need to become green after being yellow */

        checkIfWeHaveSomethingToDo();
    }

    void Scrobbler::gotScrobbleResult(ScrobbleResult result)
    {
        qDebug() << "received scrobble result:" << result;
        if (!_pendingScrobble)
        {
            qDebug() << " did not expect a scrobble result right now";
            return;
        }

        _timeoutTimer->stop();

        if (result == ScrobbleResult::Error)
        {
            reinsertPendingScrobbleAtFrontOfQueue();
            startBackoffTimer(_backend->getInitialBackoffMillisecondsForErrorReply());
            return;
        }

        _backoffMilliseconds = 0;

        auto trackPtr = _pendingScrobble;
        _pendingScrobble = nullptr;

        switch (result)
        {
            case ScrobbleResult::Success:
                trackPtr->scrobbledSuccessfully();
                break;
            case ScrobbleResult::Ignored:
                trackPtr->scrobbleIgnored();
                break;
            case ScrobbleResult::Error: /* already handled before the switch */
                break;
        }

        reevaluateStatus(); /* status may need to become green after being yellow */

        auto delayBetweenSubsequentScrobbles =
                _backend->getDelayInMillisecondsBetweenSubsequentScrobbles();

        QTimer::singleShot(delayBetweenSubsequentScrobbles, this, &Scrobbler::wakeUp);
    }

    void Scrobbler::backendStateChanged(ScrobblingBackendState newState,
                                        ScrobblingBackendState oldState)
    {
        qDebug() << "backend state changed to:" << newState;

        _timeoutTimer->stop();

        reevaluateStatus();

        /* not sure about this */
        if (oldState == ScrobblingBackendState::PermanentFatalError)
        {
            _backoffMilliseconds = 0;
        }

        /* should we wait for something to change in the backend? */
        switch (newState)
        {
            case ScrobblingBackendState::WaitingForUserCredentials:
                return; /* no waiting for timeout */
            default:
                checkIfWeHaveSomethingToDo();
                break;
        }
    }

    void Scrobbler::serviceTemporarilyUnavailable()
    {
        qDebug() << "serviceTemporarilyUnavailable() called";

        reinsertPendingScrobbleAtFrontOfQueue();

        startBackoffTimer(_backend->getInitialBackoffMillisecondsForUnavailability());
    }

    void Scrobbler::reevaluateStatus()
    {
        auto oldStatus = _status;
        auto newStatus = oldStatus;

        auto backendState = _backend->state();
        switch (backendState)
        {
            case ScrobblingBackendState::NotInitialized:
                newStatus = ScrobblerStatus::Unknown;
                break;

            case ScrobblingBackendState::ReadyForScrobbling:
                newStatus = ScrobblerStatus::Green;
                break;

            case ScrobblingBackendState::PermanentFatalError:
                newStatus = ScrobblerStatus::Red;
                break;

            case ScrobblingBackendState::WaitingForUserCredentials:
                newStatus = ScrobblerStatus::WaitingForUserCredentials;
                break;
        }

        if (newStatus == ScrobblerStatus::Green && _backoffTimer->isActive()
                && _backoffMilliseconds >= 512)
        {
            newStatus = ScrobblerStatus::Yellow;
        }

        _status = newStatus;
        if (oldStatus != newStatus)
            emit statusChanged(newStatus);
    }

    void Scrobbler::startBackoffTimer(int initialBackoffMilliseconds)
    {
        _backoffTimer->stop();

        /* make sure the starting interval is at least above zero */
        initialBackoffMilliseconds = qMax(10, initialBackoffMilliseconds);

        const int maxInterval = 5 * 60 * 1000 /* 5 minutes */;

        if (_backoffMilliseconds < initialBackoffMilliseconds)
        {
            _backoffMilliseconds = initialBackoffMilliseconds;
        }
        else if (_backoffMilliseconds < maxInterval)
        {
            _backoffMilliseconds = qMin(maxInterval, _backoffMilliseconds * 2);
        }

        qDebug() << "starting backoff timer with interval:" << _backoffMilliseconds;
        _backoffTimer->start(_backoffMilliseconds);

        reevaluateStatus(); /* status may need to become yellow */
    }

    void Scrobbler::reinsertPendingScrobbleAtFrontOfQueue()
    {
        auto trackPtr = _pendingScrobble;
        _pendingScrobble = nullptr;

        if (trackPtr)
        {
            /* reinsert at front of the queue */
            _tracksToScrobble.insert(0, trackPtr);
        }
    }

}
