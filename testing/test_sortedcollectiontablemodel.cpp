/*
    Copyright (C) 2023, Kevin Andre <hyperquantum@gmail.com>

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

#include "test_sortedcollectiontablemodel.h"

#include "client/localhashidrepository.h"

#include "gui-remote/collectiontablemodel.h"

#include <QtTest/QTest>

using namespace PMP;
using namespace PMP::Client;

namespace
{
    CollectionTrackInfo createTrack(int hashId, QString title, QString artist)
    {
        return CollectionTrackInfo(LocalHashId(hashId), true, title, artist, "", "",
                                   3 * 60 * 1000);
    }

    void logRowMovements(SortedCollectionTableModel& model, QString* output)
    {
        QObject::connect(
            &model, &SortedCollectionTableModel::rowsMoved,
            [output](const QModelIndex& sourceParent, int sourceStart, int sourceEnd,
                     const QModelIndex& destinationParent, int destinationRow)
            {
                QString s = "M";
                s += QString::number(sourceStart);
                s += "-";
                s += QString::number(sourceEnd);
                s += ">";
                s += QString::number(destinationRow);
                s += ";";

                *output += s;
            }
        );
    }

    void verifyInnerToOuterMapping(SortedCollectionTableModel& model)
    {
        for (int rowIndex = 0; rowIndex < model.rowCount(); ++rowIndex)
        {
            auto* track = model.trackAt(rowIndex);

            auto index = model.trackIndex(track->hashId());

            QCOMPARE(index, rowIndex);
        }
    }
}

void TestSortedCollectionTableModel::trackTitleUpdateCausesMoveUpward()
{
    PlayerControllerMock playerController;
    CurrentTrackMonitorMock currentTrackMonitor;
    QueueHashesMonitorMock queueHashesMonitor;
    UserForStatisticsDisplayMock userForStatisticsDisplay;
    UserDataFetcherMock userDataFetcher;
    CollectionWatcherMock collectionWatcher;

    collectionWatcher.addTrack(createTrack(1, "B", "B"));
    collectionWatcher.addTrack(createTrack(2, "D", "D"));
    collectionWatcher.addTrack(createTrack(3, "F", "F"));
    collectionWatcher.addTrack(createTrack(4, "H", "H"));
    collectionWatcher.addTrack(createTrack(5, "K", "K"));

    ServerInterfaceMock serverInterface;
    serverInterface.setUserDataFetcher(&userDataFetcher);
    serverInterface.setPlayerController(&playerController);
    serverInterface.setCollectionWatcher(&collectionWatcher);
    serverInterface.setCurrentTrackMonitor(&currentTrackMonitor);

    SortedCollectionTableModel model(nullptr, &serverInterface, &queueHashesMonitor,
                                     &userForStatisticsDisplay);

    QString movements;
    logRowMovements(model, &movements);

    QCOMPARE(model.trackAt(0)->title(), "B");
    QCOMPARE(model.trackAt(1)->title(), "D");
    QCOMPARE(model.trackAt(2)->title(), "F");
    QCOMPARE(model.trackAt(3)->title(), "H");
    verifyInnerToOuterMapping(model);

    collectionWatcher.modifyTrackTitle(LocalHashId(2), "A");

    QCOMPARE(movements, "M1-1>0;");

    QCOMPARE(model.trackAt(0)->title(), "A");
    QCOMPARE(model.trackAt(1)->title(), "B");
    QCOMPARE(model.trackAt(2)->title(), "F");
    QCOMPARE(model.trackAt(3)->title(), "H");
    verifyInnerToOuterMapping(model);
}

void TestSortedCollectionTableModel::trackTitleUpdateCausesNoMove_v1()
{
    PlayerControllerMock playerController;
    CurrentTrackMonitorMock currentTrackMonitor;
    QueueHashesMonitorMock queueHashesMonitor;
    UserForStatisticsDisplayMock userForStatisticsDisplay;
    UserDataFetcherMock userDataFetcher;
    CollectionWatcherMock collectionWatcher;

    collectionWatcher.addTrack(createTrack(1, "B", "B"));
    collectionWatcher.addTrack(createTrack(2, "D", "D"));
    collectionWatcher.addTrack(createTrack(3, "F", "F"));
    collectionWatcher.addTrack(createTrack(4, "H", "H"));
    collectionWatcher.addTrack(createTrack(5, "K", "K"));

    ServerInterfaceMock serverInterface;
    serverInterface.setUserDataFetcher(&userDataFetcher);
    serverInterface.setPlayerController(&playerController);
    serverInterface.setCollectionWatcher(&collectionWatcher);
    serverInterface.setCurrentTrackMonitor(&currentTrackMonitor);

    SortedCollectionTableModel model(nullptr, &serverInterface, &queueHashesMonitor,
                                     &userForStatisticsDisplay);

    QString movements;
    logRowMovements(model, &movements);

    QCOMPARE(model.trackAt(0)->title(), "B");
    QCOMPARE(model.trackAt(1)->title(), "D");
    QCOMPARE(model.trackAt(2)->title(), "F");
    QCOMPARE(model.trackAt(3)->title(), "H");
    verifyInnerToOuterMapping(model);

    collectionWatcher.modifyTrackTitle(LocalHashId(2), "C");

    QCOMPARE(movements, "");

    QCOMPARE(model.trackAt(0)->title(), "B");
    QCOMPARE(model.trackAt(1)->title(), "C");
    QCOMPARE(model.trackAt(2)->title(), "F");
    QCOMPARE(model.trackAt(3)->title(), "H");
    verifyInnerToOuterMapping(model);
}

void TestSortedCollectionTableModel::trackTitleUpdateCausesNoMove_v2()
{
    PlayerControllerMock playerController;
    CurrentTrackMonitorMock currentTrackMonitor;
    QueueHashesMonitorMock queueHashesMonitor;
    UserForStatisticsDisplayMock userForStatisticsDisplay;
    UserDataFetcherMock userDataFetcher;
    CollectionWatcherMock collectionWatcher;

    collectionWatcher.addTrack(createTrack(1, "B", "B"));
    collectionWatcher.addTrack(createTrack(2, "D", "D"));
    collectionWatcher.addTrack(createTrack(3, "F", "F"));
    collectionWatcher.addTrack(createTrack(4, "H", "H"));
    collectionWatcher.addTrack(createTrack(5, "K", "K"));

    ServerInterfaceMock serverInterface;
    serverInterface.setUserDataFetcher(&userDataFetcher);
    serverInterface.setPlayerController(&playerController);
    serverInterface.setCollectionWatcher(&collectionWatcher);
    serverInterface.setCurrentTrackMonitor(&currentTrackMonitor);

    SortedCollectionTableModel model(nullptr, &serverInterface, &queueHashesMonitor,
                                     &userForStatisticsDisplay);

    QString movements;
    logRowMovements(model, &movements);

    QCOMPARE(model.trackAt(0)->title(), "B");
    QCOMPARE(model.trackAt(1)->title(), "D");
    QCOMPARE(model.trackAt(2)->title(), "F");
    QCOMPARE(model.trackAt(3)->title(), "H");
    verifyInnerToOuterMapping(model);

    collectionWatcher.modifyTrackTitle(LocalHashId(2), "E");

    QCOMPARE(movements, "");

    QCOMPARE(model.trackAt(0)->title(), "B");
    QCOMPARE(model.trackAt(1)->title(), "E");
    QCOMPARE(model.trackAt(2)->title(), "F");
    QCOMPARE(model.trackAt(3)->title(), "H");
    verifyInnerToOuterMapping(model);
}

void TestSortedCollectionTableModel::trackTitleUpdateCausesMoveDownward()
{
    PlayerControllerMock playerController;
    CurrentTrackMonitorMock currentTrackMonitor;
    QueueHashesMonitorMock queueHashesMonitor;
    UserForStatisticsDisplayMock userForStatisticsDisplay;
    UserDataFetcherMock userDataFetcher;
    CollectionWatcherMock collectionWatcher;

    collectionWatcher.addTrack(createTrack(1, "B", "B"));
    collectionWatcher.addTrack(createTrack(2, "D", "D"));
    collectionWatcher.addTrack(createTrack(3, "F", "F"));
    collectionWatcher.addTrack(createTrack(4, "H", "H"));
    collectionWatcher.addTrack(createTrack(5, "K", "K"));

    ServerInterfaceMock serverInterface;
    serverInterface.setUserDataFetcher(&userDataFetcher);
    serverInterface.setPlayerController(&playerController);
    serverInterface.setCollectionWatcher(&collectionWatcher);
    serverInterface.setCurrentTrackMonitor(&currentTrackMonitor);

    SortedCollectionTableModel model(nullptr, &serverInterface, &queueHashesMonitor,
                                     &userForStatisticsDisplay);

    QString movements;
    logRowMovements(model, &movements);

    QCOMPARE(model.trackAt(0)->title(), "B");
    QCOMPARE(model.trackAt(1)->title(), "D");
    QCOMPARE(model.trackAt(2)->title(), "F");
    QCOMPARE(model.trackAt(3)->title(), "H");
    verifyInnerToOuterMapping(model);

    collectionWatcher.modifyTrackTitle(LocalHashId(2), "G");

    QCOMPARE(movements, "M1-1>3;");

    QCOMPARE(model.trackAt(0)->title(), "B");
    QCOMPARE(model.trackAt(1)->title(), "F");
    QCOMPARE(model.trackAt(2)->title(), "G");
    QCOMPARE(model.trackAt(3)->title(), "H");
    verifyInnerToOuterMapping(model);
}

void TestSortedCollectionTableModel::trackTitleUpdateCausesMoveToLastPosition()
{
    PlayerControllerMock playerController;
    CurrentTrackMonitorMock currentTrackMonitor;
    QueueHashesMonitorMock queueHashesMonitor;
    UserForStatisticsDisplayMock userForStatisticsDisplay;
    UserDataFetcherMock userDataFetcher;
    CollectionWatcherMock collectionWatcher;

    collectionWatcher.addTrack(createTrack(1, "B", "B"));
    collectionWatcher.addTrack(createTrack(2, "D", "D"));
    collectionWatcher.addTrack(createTrack(3, "F", "F"));
    collectionWatcher.addTrack(createTrack(4, "H", "H"));
    collectionWatcher.addTrack(createTrack(5, "K", "K"));

    ServerInterfaceMock serverInterface;
    serverInterface.setUserDataFetcher(&userDataFetcher);
    serverInterface.setPlayerController(&playerController);
    serverInterface.setCollectionWatcher(&collectionWatcher);
    serverInterface.setCurrentTrackMonitor(&currentTrackMonitor);

    SortedCollectionTableModel model(nullptr, &serverInterface, &queueHashesMonitor,
                                     &userForStatisticsDisplay);

    QString movements;
    logRowMovements(model, &movements);

    QCOMPARE(model.trackAt(0)->title(), "B");
    QCOMPARE(model.trackAt(1)->title(), "D");
    QCOMPARE(model.trackAt(2)->title(), "F");
    QCOMPARE(model.trackAt(3)->title(), "H");
    QCOMPARE(model.trackAt(4)->title(), "K");
    verifyInnerToOuterMapping(model);

    collectionWatcher.modifyTrackTitle(LocalHashId(2), "L");

    QCOMPARE(movements, "M1-1>5;");

    QCOMPARE(model.trackAt(0)->title(), "B");
    QCOMPARE(model.trackAt(1)->title(), "F");
    QCOMPARE(model.trackAt(2)->title(), "H");
    QCOMPARE(model.trackAt(3)->title(), "K");
    QCOMPARE(model.trackAt(4)->title(), "L");
    verifyInnerToOuterMapping(model);
}

#define NOT_IMPLEMENTED { Q_UNREACHABLE(); }

/* =========== */

PlayerState PlayerControllerMock::playerState() const
{
    return PlayerState::Stopped;
}

TriBool PlayerControllerMock::delayedStartActive() const
{
    NOT_IMPLEMENTED
}

TriBool PlayerControllerMock::isTrackPresent() const
{
    NOT_IMPLEMENTED
}

quint32 PlayerControllerMock::currentQueueId() const
{
    NOT_IMPLEMENTED
}

uint PlayerControllerMock::queueLength() const
{
    NOT_IMPLEMENTED
}

bool PlayerControllerMock::canPlay() const
{
    NOT_IMPLEMENTED
}

bool PlayerControllerMock::canPause() const
{
    NOT_IMPLEMENTED
}

bool PlayerControllerMock::canSkip() const
{
    NOT_IMPLEMENTED
}

PlayerMode PlayerControllerMock::playerMode() const
{
    NOT_IMPLEMENTED
}

quint32 PlayerControllerMock::personalModeUserId() const
{
    NOT_IMPLEMENTED
}

QString PlayerControllerMock::personalModeUserLogin() const
{
    NOT_IMPLEMENTED
}

int PlayerControllerMock::volume() const
{
    NOT_IMPLEMENTED
}

QDateTime PlayerControllerMock::delayedStartServerDeadline()
{
    NOT_IMPLEMENTED
}

SimpleFuture<ResultMessageErrorCode> PlayerControllerMock::activateDelayedStart(qint64 delayMilliseconds)
{
    NOT_IMPLEMENTED
}

SimpleFuture<ResultMessageErrorCode> PlayerControllerMock::activateDelayedStart(QDateTime startTime)
{
    NOT_IMPLEMENTED
}

SimpleFuture<ResultMessageErrorCode> PlayerControllerMock::deactivateDelayedStart()
{
    NOT_IMPLEMENTED
}

void PlayerControllerMock::play()
{
    NOT_IMPLEMENTED
}

void PlayerControllerMock::pause()
{
    NOT_IMPLEMENTED
}

void PlayerControllerMock::skip()
{
    NOT_IMPLEMENTED
}

void PlayerControllerMock::setVolume(int volume)
{
    NOT_IMPLEMENTED
}

void PlayerControllerMock::switchToPublicMode()
{
    NOT_IMPLEMENTED
}

void PlayerControllerMock::switchToPersonalMode()
{
    NOT_IMPLEMENTED
}

/* =========== */

PlayerState CurrentTrackMonitorMock::playerState() const
{
    return PlayerState::Stopped;
}

TriBool CurrentTrackMonitorMock::isTrackPresent() const
{
    NOT_IMPLEMENTED
}

quint32 CurrentTrackMonitorMock::currentQueueId() const
{
    NOT_IMPLEMENTED
}

qint64 CurrentTrackMonitorMock::currentTrackProgressMilliseconds() const
{
    NOT_IMPLEMENTED
}

LocalHashId CurrentTrackMonitorMock::currentTrackHash() const
{
    return {};
}

QString CurrentTrackMonitorMock::currentTrackTitle() const
{
    NOT_IMPLEMENTED
}

QString CurrentTrackMonitorMock::currentTrackArtist() const
{
    NOT_IMPLEMENTED
}

QString CurrentTrackMonitorMock::currentTrackPossibleFilename() const
{
    NOT_IMPLEMENTED
}

qint64 CurrentTrackMonitorMock::currentTrackLengthMilliseconds() const
{
    NOT_IMPLEMENTED
}

void CurrentTrackMonitorMock::seekTo(qint64 positionInMilliseconds)
{
    NOT_IMPLEMENTED
}

/* =========== */

bool QueueHashesMonitorMock::isPresentInQueue(PMP::Client::LocalHashId hashId) const
{
    return false;
}

/* =========== */

PMP::Nullable<quint32> UserForStatisticsDisplayMock::userId() const
{
    return 1;
}

PMP::Nullable<bool> UserForStatisticsDisplayMock::isPersonal() const
{
    return true;
}

void UserForStatisticsDisplayMock::setPersonal()
{
    //
}

void UserForStatisticsDisplayMock::setPublic()
{
    //
}

/* =========== */

void UserDataFetcherMock::enableAutoFetchForUser(quint32 userId)
{
    //
}

const UserDataFetcher::HashData* UserDataFetcherMock::getHashDataForUser(quint32 userId,
                                                                       LocalHashId hashId)
{
    NOT_IMPLEMENTED
}

/* =========== */

void CollectionWatcherMock::addTrack(CollectionTrackInfo track)
{
    _collection.insert(track.hashId(), track);
}

void CollectionWatcherMock::modifyTrackTitle(LocalHashId id, QString title)
{
    auto it = _collection.find(id);
    if (it == _collection.end())
        return;

    it.value().setTitle(title);
    Q_EMIT trackDataChanged(it.value());
}

bool CollectionWatcherMock::isAlbumArtistSupported() const
{
    NOT_IMPLEMENTED
}

void CollectionWatcherMock::enableCollectionDownloading()
{
    //
}

bool CollectionWatcherMock::downloadingInProgress() const
{
    NOT_IMPLEMENTED
}

QHash<LocalHashId, CollectionTrackInfo> CollectionWatcherMock::getCollection()
{
    return _collection;
}

CollectionTrackInfo CollectionWatcherMock::getTrack(LocalHashId hashId)
{
    NOT_IMPLEMENTED
}

/* =========== */

ServerInterfaceMock::ServerInterfaceMock()
    : _localHashIdRepository(new LocalHashIdRepository())
{
    //
}

void ServerInterfaceMock::setUserDataFetcher(UserDataFetcher* userDataFetcher)
{
    _userDataFetcher = userDataFetcher;
}

void ServerInterfaceMock::setPlayerController(PlayerController* playerController)
{
    _playerController = playerController;
}

void ServerInterfaceMock::setCollectionWatcher(CollectionWatcher* collectionWatcher)
{
    _collectionWatcher = collectionWatcher;
}

void ServerInterfaceMock::setCurrentTrackMonitor(CurrentTrackMonitor* currentTrackMonitor)
{
    _currentTrackMonitor = currentTrackMonitor;
}

PMP::Client::LocalHashIdRepository* ServerInterfaceMock::hashIdRepository() const
{
    return _localHashIdRepository;
}

AuthenticationController& ServerInterfaceMock::authenticationController()
{
    NOT_IMPLEMENTED
}

GeneralController& ServerInterfaceMock::generalController()
{
    NOT_IMPLEMENTED
}

PlayerController& ServerInterfaceMock::playerController()
{
    if (_playerController) return *_playerController;

    Q_UNREACHABLE();
}

CurrentTrackMonitor& ServerInterfaceMock::currentTrackMonitor()
{
    if (_currentTrackMonitor) return *_currentTrackMonitor;

    Q_UNREACHABLE();
}

QueueController& ServerInterfaceMock::queueController()
{
    NOT_IMPLEMENTED
}

AbstractQueueMonitor& ServerInterfaceMock::queueMonitor()
{
    NOT_IMPLEMENTED
}

QueueEntryInfoStorage& ServerInterfaceMock::queueEntryInfoStorage()
{
    NOT_IMPLEMENTED
}

QueueEntryInfoFetcher& ServerInterfaceMock::queueEntryInfoFetcher()
{
    NOT_IMPLEMENTED
}

DynamicModeController& ServerInterfaceMock::dynamicModeController()
{
    NOT_IMPLEMENTED
}

HistoryController& ServerInterfaceMock::historyController()
{
    NOT_IMPLEMENTED
}

CollectionWatcher& ServerInterfaceMock::collectionWatcher()
{
    if (_collectionWatcher) return *_collectionWatcher;

    Q_UNREACHABLE();
}

UserDataFetcher& ServerInterfaceMock::userDataFetcher()
{
    if (_userDataFetcher) return *_userDataFetcher;

    Q_UNREACHABLE();
}

bool ServerInterfaceMock::isLoggedIn() const
{
    return true;
}

quint32 ServerInterfaceMock::userLoggedInId() const
{
    return 1;
}

QString ServerInterfaceMock::userLoggedInName() const
{
    return "Username";
}

QTEST_MAIN(TestSortedCollectionTableModel)
