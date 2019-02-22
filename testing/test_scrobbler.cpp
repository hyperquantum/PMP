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

#include "test_scrobbler.h"

#include "server/scrobbler.h"

#include <QTimer>
#include <QtDebug>
#include <QtTest/QTest>
#include <QVector>

#include <memory>

using namespace PMP;

// ================================= BackendMock ================================= //

BackendMock::BackendMock(bool requireAuthentication)
 : _temporaryUnavailabilitiesToStageAtScrobbleTime(0),
   _scrobbledSuccessfullyCount(0), _tracksIgnoredCount(0),
   _requireAuthentication(requireAuthentication),
   _haveApiToken(false), _apiTokenWillBeAcceptedByApi(false)
{
    qDebug() << "running BackendMock(" << requireAuthentication << ")";
    setInitialBackoffMillisecondsForUnavailability(30);
}

void BackendMock::initialize() {
    if (!_requireAuthentication || _haveApiToken) {
        setState(ScrobblingBackendState::ReadyForScrobbling);
        return;
    }

    setState(ScrobblingBackendState::WaitingForUserCredentials);
}

void BackendMock::setTemporaryUnavailabilitiesToStageForScrobbles(int count) {
    _temporaryUnavailabilitiesToStageAtScrobbleTime = count;
}

void BackendMock::setUserCredentials(QString username, QString password) {
    _username = username;
    _password = password;

    QTimer::singleShot(10, this, SLOT(pretendAuthenticationResultReceived()));

    setState(ScrobblingBackendState::WaitingForAuthenticationResult);
}

void BackendMock::setApiToken(bool willBeAcceptedByApi) {
    _haveApiToken = true;
    _apiTokenWillBeAcceptedByApi = willBeAcceptedByApi;

    switch (state()) {
        case ScrobblingBackendState::NotInitialized:
            break;
        case ScrobblingBackendState::WaitingForUserCredentials:
        case ScrobblingBackendState::WaitingForAuthenticationResult:
        case ScrobblingBackendState::InvalidUserCredentials:
            setState(ScrobblingBackendState::ReadyForScrobbling);
            break;
        case ScrobblingBackendState::ReadyForScrobbling:
        case ScrobblingBackendState::WaitingForScrobbleResult:
            break;
        case ScrobblingBackendState::PermanentFatalError:
            break;
    }
}

void BackendMock::scrobbleTrack(QDateTime timestamp, QString const& title,
                                QString const& artist, QString const& album,
                                int trackDurationSeconds)
{
    if (state() != ScrobblingBackendState::ReadyForScrobbling)
        return;

    if (_temporaryUnavailabilitiesToStageAtScrobbleTime > 0) {
        _temporaryUnavailabilitiesToStageAtScrobbleTime--;
        emit serviceTemporarilyUnavailable();
        return;
    }

    if (_haveApiToken && !_apiTokenWillBeAcceptedByApi) {
        QTimer::singleShot(
            10, this, SLOT(pretendScrobbleFailedBecauseTokenNoLongerValid())
        );
        return;
    }

    if (timestamp.date().year() < 2018) {
        QTimer::singleShot(10, this, SLOT(pretendScrobbleFailedBecauseTrackIgnored()));
        return;
    }

    (void)title;
    (void)artist;
    (void)album;
    (void)trackDurationSeconds;

    QTimer::singleShot(10, this, SLOT(pretendSuccessfullScrobble()));
}

void BackendMock::pretendAuthenticationResultReceived() {
    if (_username == "CorrectUsername" && _password == "CorrectPassword")
        setState(ScrobblingBackendState::ReadyForScrobbling);
    else
        setState(ScrobblingBackendState::InvalidUserCredentials);
}

void BackendMock::pretendSuccessfullScrobble() {
    _scrobbledSuccessfullyCount++;
    emit gotScrobbleResult(ScrobbleResult::Success);
}

void BackendMock::pretendScrobbleFailedBecauseTokenNoLongerValid() {
    _haveApiToken = false;
    setState(ScrobblingBackendState::WaitingForUserCredentials);
    emit gotScrobbleResult(ScrobbleResult::Error);
}

void BackendMock::pretendScrobbleFailedBecauseTrackIgnored() {
    _tracksIgnoredCount++;
    emit gotScrobbleResult(ScrobbleResult::Ignored);
}

// ================================= DataProviderMock ================================= //

DataProviderMock::DataProviderMock()
{
    //
}

void DataProviderMock::add(std::shared_ptr<PMP::TrackToScrobble> track) {
    _tracksToScrobble.enqueue(track);
}

void DataProviderMock::add(QVector<std::shared_ptr<TrackToScrobbleMock>> tracks) {
    /* cannot use append because of different types, so we use foreach instead */
    Q_FOREACH(auto track, tracks) {
        add(track);
    }
}

QVector<std::shared_ptr<TrackToScrobble>> DataProviderMock::getNextTracksToScrobble() {
    auto result = _tracksToScrobble.toVector();
    _tracksToScrobble.clear();
    return result;
}

// =============================== TrackToScrobbleMock =============================== //

TrackToScrobbleMock::TrackToScrobbleMock(QDateTime timestamp, QString title,
                                         QString artist)
    : _timestamp(timestamp), _title(title), _artist(artist), _album(""),
      _scrobbled(false), _cannotBeScrobbled(false)
{
    //
}

void TrackToScrobbleMock::scrobbledSuccessfully() {
    qDebug() << "track scrobbled successfully";

    QVERIFY(!_scrobbled);
    QVERIFY(!_cannotBeScrobbled);

    _scrobbled = true;
    _scrobbledTimestamp = QDateTime::currentDateTimeUtc();
}

void TrackToScrobbleMock::scrobbleIgnored() {
    qDebug() << "track was ignored by scrobbling provider";

    QVERIFY(!_scrobbled);
    QVERIFY(!_cannotBeScrobbled);

    _cannotBeScrobbled = true;
}

// ================================= TestScrobbler ================================= //

void TestScrobbler::trivialScrobble() {
    DataProviderMock dataProvider;
    auto track = addTrackToScrobble(dataProvider);

    QVERIFY(!track->scrobbled());

    auto backend = new BackendMock(false);
    Scrobbler scrobbler(nullptr, &dataProvider, backend);
    scrobbler.wakeUp();

    QTRY_VERIFY(track->scrobbled());
    QCOMPARE(backend->scrobbledSuccessfullyCount(), 1);
    QTRY_COMPARE(backend->state(), ScrobblingBackendState::ReadyForScrobbling);
}

void TestScrobbler::multipleSimpleScrobbles() {
    DataProviderMock dataProvider;

    QVector<std::shared_ptr<TrackToScrobbleMock>> tracks;
    auto time = makeDateTime(2018, 4, 9, 23, 30);

    tracks << addTrackToScrobble(dataProvider, time, "Title 1", "Artist 1");
    time = time.addSecs(185);

    tracks << addTrackToScrobble(dataProvider, time, "Title 2", "Artist 2");
    time = time.addSecs(180);

    tracks << addTrackToScrobble(dataProvider, time, "Title 3", "Artist 3");
    time = time.addSecs(203);

    tracks << addTrackToScrobble(dataProvider, time, "Title 4", "Artist 4");
    time = time.addSecs(189);

    tracks << addTrackToScrobble(dataProvider, time, "Title 5", "Artist 5");

    Q_FOREACH(auto track, tracks) {
        QVERIFY(!track->scrobbled());
    }

    auto backend = new BackendMock(false);
    Scrobbler scrobbler(nullptr, &dataProvider, backend);
    scrobbler.wakeUp();

    Q_FOREACH(auto track, tracks) {
        QTRY_VERIFY(track->scrobbled());
    }

    QCOMPARE(backend->scrobbledSuccessfullyCount(), 5);
    QTRY_COMPARE(backend->state(), ScrobblingBackendState::ReadyForScrobbling);
}

void TestScrobbler::scrobbleWithAuthentication() {
    DataProviderMock dataProvider;
    auto track = addTrackToScrobble(dataProvider);

    QVERIFY(!track->scrobbled());

    auto backend = new BackendMock(true);
    Scrobbler scrobbler(nullptr, &dataProvider, backend);
    scrobbler.wakeUp();

    QTRY_COMPARE(backend->state(), ScrobblingBackendState::WaitingForUserCredentials);

    backend->setUserCredentials("CorrectUsername", "CorrectPassword");

    QTRY_VERIFY(track->scrobbled());
    QCOMPARE(backend->scrobbledSuccessfullyCount(), 1);
    QTRY_COMPARE(backend->state(), ScrobblingBackendState::ReadyForScrobbling);
}

void TestScrobbler::scrobbleWithExistingValidToken() {
    DataProviderMock dataProvider;
    auto track = addTrackToScrobble(dataProvider);

    QVERIFY(!track->scrobbled());

    auto backend = new BackendMock(true);
    backend->setApiToken(true); /* set an active, valid token */
    Scrobbler scrobbler(nullptr, &dataProvider, backend);
    scrobbler.wakeUp();

    QTRY_VERIFY(track->scrobbled());
    QCOMPARE(backend->scrobbledSuccessfullyCount(), 1);
    QTRY_COMPARE(backend->state(), ScrobblingBackendState::ReadyForScrobbling);
}

void TestScrobbler::scrobbleWithTokenChangeAfterInvalidToken() {
    DataProviderMock dataProvider;
    auto track = addTrackToScrobble(dataProvider);

    QVERIFY(!track->scrobbled());

    auto backend = new BackendMock(true);
    backend->setApiToken(false); /* set an active, but invalid, token */
    Scrobbler scrobbler(nullptr, &dataProvider, backend);
    scrobbler.wakeUp();

    /* first wait for the initialization to complete */
    QTRY_COMPARE(backend->state(), ScrobblingBackendState::ReadyForScrobbling);

    /* now wait for the backend to realize that the token is not valid */
    QTRY_COMPARE(backend->state(), ScrobblingBackendState::WaitingForUserCredentials);

    backend->setApiToken(true); /* set a valid token */

    QTRY_VERIFY(track->scrobbled());
    QCOMPARE(backend->scrobbledSuccessfullyCount(), 1);
    QTRY_COMPARE(backend->state(), ScrobblingBackendState::ReadyForScrobbling);
}

void TestScrobbler::mustSkipScrobblesThatAreTooOld() {
    DataProviderMock dataProvider;

    QVector<std::shared_ptr<TrackToScrobbleMock>> tracks;
    auto time = makeDateTime(2017, 12, 31, 23, 30);

    for (int i = 0; i < 3; ++i) {
        tracks << addTrackToScrobble(dataProvider, time);
        QVERIFY(tracks.last()->scrobbled() == false);
        QVERIFY(tracks.last()->ignored() == false);
        time = time.addSecs(300);
    }

    time = makeDateTime(2018, 1, 1, 0, 5);

    for (int i = 0; i < 5; ++i) {
        tracks << addTrackToScrobble(dataProvider, time);
        QVERIFY(tracks.last()->scrobbled() == false);
        QVERIFY(tracks.last()->ignored() == false);
        time = time.addSecs(300);
    }

    auto backend = new BackendMock(false);
    Scrobbler scrobbler(nullptr, &dataProvider, backend);
    scrobbler.wakeUp();

    for (int i = 0; i < tracks.size(); ++i) {
        if (i < 3) {
            QTRY_VERIFY(tracks[i]->ignored());
        }
        else {
            QTRY_VERIFY(tracks[i]->scrobbled());
        }
    }

    QCOMPARE(backend->tracksIgnoredCount(), 3);
    QCOMPARE(backend->scrobbledSuccessfullyCount(), 5);
    QTRY_COMPARE(backend->state(), ScrobblingBackendState::ReadyForScrobbling);
}

void TestScrobbler::retriesAfterTemporaryUnavailability() {
    DataProviderMock dataProvider;
    auto track1 = addTrackToScrobble(dataProvider);
    auto track2 = addTrackToScrobble(dataProvider);

    QVERIFY(!track1->scrobbled());
    QVERIFY(!track2->scrobbled());

    auto backend = new BackendMock(false);
    backend->setTemporaryUnavailabilitiesToStageForScrobbles(3);
    Scrobbler scrobbler(nullptr, &dataProvider, backend);
    scrobbler.wakeUp();

    QTRY_VERIFY(track2->scrobbled());
    QVERIFY(track1->scrobbled());
    QVERIFY(track1->scrobbledTimestamp() < track2->scrobbledTimestamp());
    QCOMPARE(backend->scrobbledSuccessfullyCount(), 2);
    QTRY_COMPARE(backend->state(), ScrobblingBackendState::ReadyForScrobbling);
}

QDateTime TestScrobbler::makeDateTime(int year, int month, int day,
                                      int hours, int minutes)
{
    return QDateTime(QDate(year, month, day), QTime(hours, minutes));
}

std::shared_ptr<TrackToScrobbleMock> TestScrobbler::addTrackToScrobble(
                                                           DataProviderMock& dataProvider)
{
    return addTrackToScrobble(dataProvider, makeDateTime(2018, 10, 10, 17, 33));
}

std::shared_ptr<TrackToScrobbleMock> TestScrobbler::addTrackToScrobble(
                                           DataProviderMock& dataProvider, QDateTime time)
{
    return addTrackToScrobble(dataProvider, time, "Title", "Artist");
}

std::shared_ptr<TrackToScrobbleMock> TestScrobbler::addTrackToScrobble(
                                                           DataProviderMock& dataProvider,
                                                           QDateTime time,
                                                           QString title, QString artist)
{
    auto track = std::make_shared<TrackToScrobbleMock>(time, title, artist);
    dataProvider.add(track);
    return track;
}

QTEST_MAIN(TestScrobbler)
