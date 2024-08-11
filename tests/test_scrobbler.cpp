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

#include "test_scrobbler.h"

#include "server/scrobbler.h"

#include <QTimer>
#include <QtDebug>
#include <QtTest/QTest>
#include <QVector>

// ================================= BackendMock ================================= //

BackendMock::BackendMock(bool requireAuthentication)
 : _nowPlayingUpdatedCount(0),
   _temporaryUnavailabilitiesToStageAtScrobbleTime(0),
   _scrobbledSuccessfullyCount(0), _tracksIgnoredCount(0),
   _requireAuthentication(requireAuthentication),
   _haveApiToken(false), _apiTokenWillBeAcceptedByApi(false)
{
    qDebug() << "running BackendMock(" << requireAuthentication << ")";
    setDelayInMillisecondsBetweenSubsequentScrobbles(10);
    setInitialBackoffMillisecondsForUnavailability(30);
    setInitialBackoffMillisecondsForErrorReply(30);
}

NewSimpleFuture<Result> BackendMock::authenticateWithCredentials(QString usernameOrEmail,
                                                              QString password)
{
    Q_UNUSED(usernameOrEmail);
    Q_UNUSED(password);

    return FutureResult(Error::notImplemented());
}

void BackendMock::initialize()
{
    if (!_requireAuthentication || _haveApiToken)
    {
        setState(ScrobblingBackendState::ReadyForScrobbling);
        return;
    }

    setState(ScrobblingBackendState::WaitingForUserCredentials);
}

void BackendMock::setTemporaryUnavailabilitiesToStageForScrobbles(int count)
{
    _temporaryUnavailabilitiesToStageAtScrobbleTime = count;
}

void BackendMock::setUserCredentials(QString username, QString password)
{
    _username = username;
    _password = password;

    QTimer::singleShot(10, this, SLOT(pretendAuthenticationResultReceived()));
}

void BackendMock::setApiToken(bool willBeAcceptedByApi)
{
    _haveApiToken = true;
    _apiTokenWillBeAcceptedByApi = willBeAcceptedByApi;

    switch (state())
    {
        case ScrobblingBackendState::NotInitialized:
            break;
        case ScrobblingBackendState::WaitingForUserCredentials:
            setState(ScrobblingBackendState::ReadyForScrobbling);
            break;
        case ScrobblingBackendState::ReadyForScrobbling:
            break;
        case ScrobblingBackendState::PermanentFatalError:
            break;
    }
}

void BackendMock::updateNowPlaying(ScrobblingTrack track)
{
    if (state() != ScrobblingBackendState::ReadyForScrobbling)
        return;

    (void)track;

    QTimer::singleShot(10, this, &BackendMock::pretendSuccessfulNowPlaying);
}

void BackendMock::scrobbleTrack(QDateTime timestamp, ScrobblingTrack track)
{
    if (state() != ScrobblingBackendState::ReadyForScrobbling)
        return;

    if (_temporaryUnavailabilitiesToStageAtScrobbleTime > 0)
    {
        _temporaryUnavailabilitiesToStageAtScrobbleTime--;
        Q_EMIT serviceTemporarilyUnavailable();
        return;
    }

    if (_haveApiToken && !_apiTokenWillBeAcceptedByApi)
    {
        QTimer::singleShot(
            10, this, SLOT(pretendScrobbleFailedBecauseTokenNoLongerValid())
        );
        return;
    }

    if (timestamp.date().year() < 2018)
    {
        QTimer::singleShot(10, this, SLOT(pretendScrobbleFailedBecauseTrackIgnored()));
        return;
    }

    (void)track;

    QTimer::singleShot(10, this, &BackendMock::pretendSuccessfulScrobble);
}

void BackendMock::pretendAuthenticationResultReceived()
{
    if (_username == "CorrectUsername" && _password == "CorrectPassword")
        setState(ScrobblingBackendState::ReadyForScrobbling);
    else
        setState(ScrobblingBackendState::WaitingForUserCredentials);
}

void BackendMock::pretendSuccessfulNowPlaying()
{
    _nowPlayingUpdatedCount++;
    Q_EMIT gotNowPlayingResult(true);
}

void BackendMock::pretendSuccessfulScrobble()
{
    _scrobbledSuccessfullyCount++;
    Q_EMIT gotScrobbleResult(ScrobbleResult::Success);
}

void BackendMock::pretendScrobbleFailedBecauseTokenNoLongerValid()
{
    _haveApiToken = false;
    setState(ScrobblingBackendState::WaitingForUserCredentials);
    Q_EMIT gotScrobbleResult(ScrobbleResult::Error);
}

void BackendMock::pretendScrobbleFailedBecauseTrackIgnored()
{
    _tracksIgnoredCount++;
    Q_EMIT gotScrobbleResult(ScrobbleResult::Ignored);
}

// ================================= DataProviderMock ================================= //

DataProviderMock::DataProviderMock()
{
    //
}

void DataProviderMock::add(QSharedPointer<TrackToScrobble> track)
{
    _tracksToScrobble.enqueue(track);
}

void DataProviderMock::add(QVector<QSharedPointer<TrackToScrobbleMock>> tracks)
{
    /* cannot use append because of different types, so we use foreach instead */
    for (auto& track : qAsConst(tracks))
    {
        add(track);
    }
}

QVector<QSharedPointer<TrackToScrobble>> DataProviderMock::getNextTracksToScrobble()
{
    auto result = _tracksToScrobble.toVector();
    _tracksToScrobble.clear();
    return result;
}

// ============================== TrackInfoProviderMock ============================== //

TrackInfoProviderMock::TrackInfoProviderMock()
{
    /* create and register the default track to test with */

    CollectionTrackInfo track(FileHash(), true, "Title", "Artist", "Album", "AlbumArtist",
                              /* length (s):*/ 3 * 60 * 1000);

    _tracks.insert(1, track);
}

NewFuture<CollectionTrackInfo, FailureType> TrackInfoProviderMock::getTrackInfoAsync(
                                                                              uint hashId)
{
    Q_ASSERT(hashId > 0);
    Q_ASSERT(_tracks.contains(hashId));

    return FutureResult(_tracks[hashId]);
}

void TrackInfoProviderMock::registerTrack(uint hashId, QString title, QString artist)
{
    registerTrack(hashId, title, artist, "", "");
}

void TrackInfoProviderMock::registerTrack(uint hashId, QString title, QString artist,
                                          QString album, QString albumArtist)
{
    Q_ASSERT(hashId > 0);
    Q_ASSERT(!_tracks.contains(hashId));

    CollectionTrackInfo track(FileHash(), true, title, artist, album, albumArtist,
                              /* length (s):*/ 4 * 60 * 1000);

    _tracks.insert(hashId, track);
}

// =============================== TrackToScrobbleMock =============================== //

TrackToScrobbleMock::TrackToScrobbleMock(QDateTime timestamp, uint hashId)
  : _timestamp(timestamp),
    _hashId(hashId),
    _scrobbled(false),
    _cannotBeScrobbled(false)
{
    //
}

void TrackToScrobbleMock::scrobbledSuccessfully()
{
    qDebug() << "track scrobbled successfully";

    QVERIFY(!_scrobbled);
    QVERIFY(!_cannotBeScrobbled);

    _scrobbled = true;
    _scrobbledTimestamp = QDateTime::currentDateTimeUtc();
}

void TrackToScrobbleMock::scrobbleIgnored()
{
    qDebug() << "track was ignored by scrobbling provider";

    QVERIFY(!_scrobbled);
    QVERIFY(!_cannotBeScrobbled);

    _cannotBeScrobbled = true;
}

// ================================= TestScrobbler ================================= //

void TestScrobbler::simpleNowPlayingUpdate()
{
    TrackInfoProviderMock trackInfoProvider;
    auto dataProvider = QSharedPointer<DataProviderMock>::create();
    auto backend = new BackendMock(false);
    Scrobbler scrobbler(nullptr, dataProvider, backend, &trackInfoProvider);

    QCOMPARE(backend->nowPlayingUpdatedCount(), 0);

    scrobbler.nowPlayingTrack(makeDateTime(2019, 6, 9, 19, 12), createTrack());

    QTRY_COMPARE(backend->nowPlayingUpdatedCount(), 1);
}

void TestScrobbler::nowPlayingWithAuthentication()
{
    TrackInfoProviderMock trackInfoProvider;
    auto dataProvider = QSharedPointer<DataProviderMock>::create();
    auto backend = new BackendMock(true);
    Scrobbler scrobbler(nullptr, dataProvider, backend, &trackInfoProvider);

    QCOMPARE(backend->nowPlayingUpdatedCount(), 0);

    scrobbler.nowPlayingTrack(makeDateTime(2019, 6, 9, 19, 12), createTrack());

    QTRY_COMPARE(backend->state(), ScrobblingBackendState::WaitingForUserCredentials);
    QCOMPARE(backend->nowPlayingUpdatedCount(), 0);

    backend->setUserCredentials("CorrectUsername", "CorrectPassword");

    QTRY_COMPARE(backend->nowPlayingUpdatedCount(), 1);
}

void TestScrobbler::nowPlayingWithTrackToScrobble()
{
    TrackInfoProviderMock trackInfoProvider;
    auto dataProvider = QSharedPointer<DataProviderMock>::create();
    auto track = addTrackToScrobble(dataProvider);

    QVERIFY(!track->scrobbled());

    auto backend = new BackendMock(false);
    Scrobbler scrobbler(nullptr, dataProvider, backend, &trackInfoProvider);

    QCOMPARE(backend->nowPlayingUpdatedCount(), 0);

    scrobbler.nowPlayingTrack(makeDateTime(2019, 6, 9, 19, 12), createTrack());

    QTRY_COMPARE(backend->nowPlayingUpdatedCount(), 1);

    QTRY_VERIFY(track->scrobbled());
}

void TestScrobbler::nowPlayingWithImmediateScrobble()
{
    TrackInfoProviderMock trackInfoProvider;
    auto dataProvider = QSharedPointer<DataProviderMock>::create();
    auto backend = new BackendMock(false);
    Scrobbler scrobbler(nullptr, dataProvider, backend, &trackInfoProvider);

    QCOMPARE(backend->nowPlayingUpdatedCount(), 0);

    scrobbler.nowPlayingTrack(makeDateTime(2019, 6, 9, 19, 12), createTrack());
    // no delay here
    auto track = addTrackToScrobble(dataProvider);
    scrobbler.wakeUp();

    QTRY_COMPARE(backend->nowPlayingUpdatedCount(), 1);

    QTRY_VERIFY(track->scrobbled());
}

void TestScrobbler::trivialScrobble()
{
    TrackInfoProviderMock trackInfoProvider;
    auto dataProvider = QSharedPointer<DataProviderMock>::create();
    auto track = addTrackToScrobble(dataProvider);

    QVERIFY(!track->scrobbled());

    auto backend = new BackendMock(false);
    Scrobbler scrobbler(nullptr, dataProvider, backend, &trackInfoProvider);
    scrobbler.wakeUp();

    QTRY_VERIFY(track->scrobbled());
    QCOMPARE(backend->scrobbledSuccessfullyCount(), 1);
    QTRY_COMPARE(backend->state(), ScrobblingBackendState::ReadyForScrobbling);
}

void TestScrobbler::multipleSimpleScrobbles()
{
    TrackInfoProviderMock trackInfoProvider;
    auto dataProvider = QSharedPointer<DataProviderMock>::create();

    QVector<QSharedPointer<TrackToScrobbleMock>> tracks;
    auto time = makeDateTime(2018, 4, 9, 23, 30);

    tracks << addTrackToScrobble(dataProvider, trackInfoProvider, time, 2,
                                 "Title 1", "Artist 1");
    time = time.addSecs(185);

    tracks << addTrackToScrobble(dataProvider, trackInfoProvider, time, 3,
                                 "Title 2", "Artist 2");
    time = time.addSecs(180);

    tracks << addTrackToScrobble(dataProvider, trackInfoProvider, time, 4,
                                 "Title 3", "Artist 3");
    time = time.addSecs(203);

    tracks << addTrackToScrobble(dataProvider, trackInfoProvider, time, 5,
                                 "Title 4", "Artist 4");
    time = time.addSecs(189);

    tracks << addTrackToScrobble(dataProvider, trackInfoProvider, time, 6,
                                 "Title 5", "Artist 5");

    for (auto& track : qAsConst(tracks))
    {
        QVERIFY(!track->scrobbled());
    }

    auto backend = new BackendMock(false);
    Scrobbler scrobbler(nullptr, dataProvider, backend, &trackInfoProvider);
    scrobbler.wakeUp();

    for (auto& track : qAsConst(tracks))
    {
        QTRY_VERIFY(track->scrobbled());
    }

    QCOMPARE(backend->scrobbledSuccessfullyCount(), 5);
    QTRY_COMPARE(backend->state(), ScrobblingBackendState::ReadyForScrobbling);
}

void TestScrobbler::scrobbleWithAuthentication()
{
    TrackInfoProviderMock trackInfoProvider;
    auto dataProvider = QSharedPointer<DataProviderMock>::create();
    auto track = addTrackToScrobble(dataProvider);

    QVERIFY(!track->scrobbled());

    auto backend = new BackendMock(true);
    Scrobbler scrobbler(nullptr, dataProvider, backend, &trackInfoProvider);
    scrobbler.wakeUp();

    QTRY_COMPARE(backend->state(), ScrobblingBackendState::WaitingForUserCredentials);

    backend->setUserCredentials("CorrectUsername", "CorrectPassword");

    QTRY_VERIFY(track->scrobbled());
    QCOMPARE(backend->scrobbledSuccessfullyCount(), 1);
    QTRY_COMPARE(backend->state(), ScrobblingBackendState::ReadyForScrobbling);
}

void TestScrobbler::scrobbleWithExistingValidToken()
{
    TrackInfoProviderMock trackInfoProvider;
    auto dataProvider = QSharedPointer<DataProviderMock>::create();
    auto track = addTrackToScrobble(dataProvider);

    QVERIFY(!track->scrobbled());

    auto backend = new BackendMock(true);
    backend->setApiToken(true); /* set an active, valid token */
    Scrobbler scrobbler(nullptr, dataProvider, backend, &trackInfoProvider);
    scrobbler.wakeUp();

    QTRY_VERIFY(track->scrobbled());
    QCOMPARE(backend->scrobbledSuccessfullyCount(), 1);
    QTRY_COMPARE(backend->state(), ScrobblingBackendState::ReadyForScrobbling);
}

void TestScrobbler::scrobbleWithTokenChangeAfterInvalidToken()
{
    TrackInfoProviderMock trackInfoProvider;
    auto dataProvider = QSharedPointer<DataProviderMock>::create();
    auto track = addTrackToScrobble(dataProvider);

    QVERIFY(!track->scrobbled());

    auto backend = new BackendMock(true);
    backend->setApiToken(false); /* set an active, but invalid, token */
    Scrobbler scrobbler(nullptr, dataProvider, backend, &trackInfoProvider);
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

void TestScrobbler::mustSkipScrobblesThatAreTooOld()
{
    TrackInfoProviderMock trackInfoProvider;
    auto dataProvider = QSharedPointer<DataProviderMock>::create();

    QVector<QSharedPointer<TrackToScrobbleMock>> tracks;
    auto time = makeDateTime(2017, 12, 31, 23, 30);

    for (int i = 0; i < 3; ++i)
    {
        tracks << addTrackToScrobble(dataProvider, time);
        QVERIFY(tracks.last()->scrobbled() == false);
        QVERIFY(tracks.last()->ignored() == false);
        time = time.addSecs(300);
    }

    time = makeDateTime(2018, 1, 1, 0, 5);

    for (int i = 0; i < 5; ++i)
    {
        tracks << addTrackToScrobble(dataProvider, time);
        QVERIFY(tracks.last()->scrobbled() == false);
        QVERIFY(tracks.last()->ignored() == false);
        time = time.addSecs(300);
    }

    auto backend = new BackendMock(false);
    Scrobbler scrobbler(nullptr, dataProvider, backend, &trackInfoProvider);
    scrobbler.wakeUp();

    for (int i = 0; i < tracks.size(); ++i)
    {
        if (i < 3)
        {
            QTRY_VERIFY(tracks[i]->ignored());
        }
        else
        {
            QTRY_VERIFY(tracks[i]->scrobbled());
        }
    }

    QCOMPARE(backend->tracksIgnoredCount(), 3);
    QCOMPARE(backend->scrobbledSuccessfullyCount(), 5);
    QTRY_COMPARE(backend->state(), ScrobblingBackendState::ReadyForScrobbling);
}

void TestScrobbler::retriesAfterTemporaryUnavailability()
{
    TrackInfoProviderMock trackInfoProvider;
    auto dataProvider = QSharedPointer<DataProviderMock>::create();
    auto track1 = addTrackToScrobble(dataProvider);
    auto track2 = addTrackToScrobble(dataProvider);

    QVERIFY(!track1->scrobbled());
    QVERIFY(!track2->scrobbled());

    auto backend = new BackendMock(false);
    backend->setTemporaryUnavailabilitiesToStageForScrobbles(3);
    Scrobbler scrobbler(nullptr, dataProvider, backend, &trackInfoProvider);
    scrobbler.wakeUp();

    QTRY_VERIFY(track2->scrobbled());
    QVERIFY(track1->scrobbled());
    QVERIFY(track1->scrobbledTimestamp() < track2->scrobbledTimestamp());
    QCOMPARE(backend->scrobbledSuccessfullyCount(), 2);
    QTRY_COMPARE(backend->state(), ScrobblingBackendState::ReadyForScrobbling);
}

ScrobblingTrack TestScrobbler::createTrack()
{
    return ScrobblingTrack("Title", "Artist", "Album name", "Album artist");
}

QDateTime TestScrobbler::makeDateTime(int year, int month, int day,
                                      int hours, int minutes)
{
    return QDateTime(QDate(year, month, day), QTime(hours, minutes));
}

QSharedPointer<TrackToScrobbleMock> TestScrobbler::addTrackToScrobble(
                                            QSharedPointer<DataProviderMock> dataProvider)
{
    auto timestamp = makeDateTime(2018, 10, 10, 17, 33);

    return addTrackToScrobble(dataProvider, timestamp);
}

QSharedPointer<TrackToScrobbleMock> TestScrobbler::addTrackToScrobble(
                                            QSharedPointer<DataProviderMock> dataProvider,
                                            QDateTime time)
{
    auto hashId = 1;

    auto track = QSharedPointer<TrackToScrobbleMock>::create(time, hashId);
    dataProvider->add(track);
    return track;
}

QSharedPointer<TrackToScrobbleMock> TestScrobbler::addTrackToScrobble(
                                            QSharedPointer<DataProviderMock> dataProvider,
                                                TrackInfoProviderMock& trackInfoProvider,
                                                           QDateTime time, uint hashId,
                                                           QString title, QString artist)
{
    trackInfoProvider.registerTrack(hashId, title, artist);

    auto track = QSharedPointer<TrackToScrobbleMock>::create(time, hashId);
    dataProvider->add(track);
    return track;
}

QTEST_MAIN(TestScrobbler)
