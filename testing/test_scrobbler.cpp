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

#include "test_scrobbler.h"

#include "server/scrobbler.h"

#include <QTimer>
#include <QtDebug>
#include <QtTest/QTest>
#include <QVector>

#include <memory>

using namespace PMP;

// ================================= BackendMock ================================= //

BackendMock::BackendMock()
 : _scrobbledSuccessfullyCount(0)
{
    qDebug() << "running BackendMock()";
}

void BackendMock::initialize() {
    setState(ScrobblingBackendState::ReadyForScrobbling);
}

void BackendMock::scrobbleTrack(QDateTime timestamp, QString const& title,
                                QString const& artist, QString const& album,
                                int trackDurationSeconds)
{
    QTimer::singleShot(0, this, SLOT(pretendSuccessfullScrobble()));
}

void BackendMock::pretendSuccessfullScrobble() {
    _scrobbledSuccessfullyCount++;
    emit gotScrobbleResult(ScrobbleResult::Success);
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
      _scrobbled(false)
{
    //
}

void TrackToScrobbleMock::scrobbledSuccessfully() {
    qDebug() << "track scrobbled successfully";
    _scrobbled = true;
}

void TrackToScrobbleMock::cannotBeScrobbled() {
    qDebug() << "track could not be scrobbled";
}

// ================================= TestScrobbler ================================= //

void TestScrobbler::trivialScrobble() {
    DataProviderMock dataProvider;

    auto track =
        std::make_shared<TrackToScrobbleMock>(
            QDateTime(QDate(2018, 4, 4), QTime(1, 30)), "Title", "Artist"
        );
    dataProvider.add(track);

    QVERIFY(!track->scrobbled());

    auto backend = new BackendMock();
    Scrobbler scrobbler(nullptr, &dataProvider, backend);
    scrobbler.wakeUp();

    QTRY_VERIFY(track->scrobbled());
    QCOMPARE(backend->scrobbledSuccessfullyCount(), 1);
}

void TestScrobbler::multipleSimpleScrobbles() {
    DataProviderMock dataProvider;

    QDateTime time(QDate(2018, 4, 9), QTime(23, 30));

    QVector<std::shared_ptr<TrackToScrobbleMock>> tracks;

    tracks << std::make_shared<TrackToScrobbleMock>(time, "Title 1", "Artist 1");
    time.addSecs(185);

    tracks << std::make_shared<TrackToScrobbleMock>(time, "Title 2", "Artist 2");
    time.addSecs(180);

    tracks << std::make_shared<TrackToScrobbleMock>(time, "Title 3", "Artist 3");
    time.addSecs(203);

    tracks << std::make_shared<TrackToScrobbleMock>(time, "Title 4", "Artist 4");
    time.addSecs(189);

    tracks << std::make_shared<TrackToScrobbleMock>(time, "Title 5", "Artist 5");

    dataProvider.add(tracks);

    Q_FOREACH(auto track, tracks) {
        QVERIFY(!track->scrobbled());
    }

    auto backend = new BackendMock();
    Scrobbler scrobbler(nullptr, &dataProvider, backend);
    scrobbler.wakeUp();

    Q_FOREACH(auto track, tracks) {
        QTRY_VERIFY(track->scrobbled());
    }

    QCOMPARE(backend->scrobbledSuccessfullyCount(), 5);
}

QTEST_MAIN(TestScrobbler)
