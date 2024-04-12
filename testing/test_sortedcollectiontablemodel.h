/*
    Copyright (C) 2023-2024, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_TESTSORTEDCOLLECTIONTABLEMODEL_H
#define PMP_TESTSORTEDCOLLECTIONTABLEMODEL_H

#include "client/collectionwatcher.h"
#include "client/currenttrackmonitor.h"
#include "client/playercontroller.h"
#include "client/queuehashesmonitor.h"
#include "client/serverinterface.h"
#include "client/userdatafetcher.h"

#include "gui-remote/userforstatisticsdisplay.h"

#include <QObject>

using namespace PMP;
using namespace PMP::Client;

class PlayerControllerMock : public PMP::Client::PlayerController
{
    Q_OBJECT
public:
    PlayerState playerState() const override;
    TriBool delayedStartActive() const override;
    TriBool isTrackPresent() const override;
    quint32 currentQueueId() const override;
    uint queueLength() const override;
    bool canPlay() const override;
    bool canPause() const override;
    bool canSkip() const override;

    PlayerMode playerMode() const override;
    quint32 personalModeUserId() const override;
    QString personalModeUserLogin() const override;

    int volume() const override;

    QDateTime delayedStartServerDeadline() override;
    SimpleFuture<AnyResultMessageCode> activateDelayedStart(
        qint64 delayMilliseconds) override;
    SimpleFuture<AnyResultMessageCode> activateDelayedStart(
        QDateTime startTime) override;
    SimpleFuture<AnyResultMessageCode> deactivateDelayedStart() override;

public Q_SLOTS:
    void play() override;
    void pause() override;
    void skip() override;

    void setVolume(int volume) override;

    void switchToPublicMode() override;
    void switchToPersonalMode() override;
};

class CurrentTrackMonitorMock : public CurrentTrackMonitor
{
    Q_OBJECT
public:
    PlayerState playerState() const override;

    TriBool isTrackPresent() const override;
    quint32 currentQueueId() const override;
    qint64 currentTrackProgressMilliseconds() const override;

    LocalHashId currentTrackHash() const override;

    QString currentTrackTitle() const override;
    QString currentTrackArtist() const override;
    QString currentTrackPossibleFilename() const override;
    qint64 currentTrackLengthMilliseconds() const override;

public Q_SLOTS:
    void seekTo(qint64 positionInMilliseconds) override;
};

class QueueHashesMonitorMock : public QueueHashesMonitor
{
    Q_OBJECT
public:
    bool isPresentInQueue(LocalHashId hashId) const override;
};

class UserForStatisticsDisplayMock : public UserForStatisticsDisplay
{
    Q_OBJECT
public:
    Nullable<quint32> userId() const override;
    Nullable<bool> isPersonal() const override;

    void setPersonal() override;
    void setPublic() override;
};

class UserDataFetcherMock : public UserDataFetcher
{
    Q_OBJECT
public:
    void enableAutoFetchForUser(quint32 userId) override;

    HashData const* getHashDataForUser(quint32 userId,
                                       PMP::Client::LocalHashId hashId) override;
};

class CollectionWatcherMock : public CollectionWatcher
{
    Q_OBJECT
public:
    void addTrack(CollectionTrackInfo track);
    void modifyTrackTitle(LocalHashId id, QString title);

    bool isAlbumArtistSupported() const override;

    void enableCollectionDownloading() override;
    bool downloadingInProgress() const override;

    QHash<LocalHashId, CollectionTrackInfo> getCollection() override;
    Nullable<CollectionTrackInfo> getTrackFromCache(LocalHashId hashId) override;
    Future<CollectionTrackInfo, AnyResultMessageCode> getTrackInfo(
                                                             LocalHashId hashId) override;
    Future<CollectionTrackInfo, AnyResultMessageCode> getTrackInfo(
                                                        FileHash const& hash) override;

private:
    QHash<LocalHashId, CollectionTrackInfo> _collection;
};

class ServerInterfaceMock : public PMP::Client::ServerInterface
{
    Q_OBJECT
public:
    ServerInterfaceMock();

    void setUserDataFetcher(UserDataFetcher* userDataFetcher);
    void setPlayerController(PlayerController* playerController);
    void setCollectionWatcher(CollectionWatcher* collectionWatcher);
    void setCurrentTrackMonitor(CurrentTrackMonitor* currentTrackMonitor);

    PMP::Client::LocalHashIdRepository* hashIdRepository() const override;

    PMP::Client::AuthenticationController& authenticationController() override;

    PMP::Client::GeneralController& generalController() override;

    PMP::Client::PlayerController& playerController() override;
    PMP::Client::CurrentTrackMonitor& currentTrackMonitor() override;

    PMP::Client::QueueController& queueController() override;
    PMP::Client::AbstractQueueMonitor& queueMonitor() override;
    PMP::Client::QueueEntryInfoStorage& queueEntryInfoStorage() override;
    PMP::Client::QueueEntryInfoFetcher& queueEntryInfoFetcher() override;

    PMP::Client::DynamicModeController& dynamicModeController() override;

    PMP::Client::HistoryController& historyController() override;

    PMP::Client::CollectionWatcher& collectionWatcher() override;
    PMP::Client::UserDataFetcher& userDataFetcher() override;

    ScrobblingController& scrobblingController() override;

    bool isLoggedIn() const override;
    quint32 userLoggedInId() const override;
    QString userLoggedInName() const override;

    bool connected() const override { return true; }

private:
    LocalHashIdRepository* _localHashIdRepository;
    UserDataFetcher* _userDataFetcher { nullptr };
    PlayerController* _playerController { nullptr };
    CollectionWatcher* _collectionWatcher { nullptr };
    CurrentTrackMonitor* _currentTrackMonitor { nullptr };
};

class TestSortedCollectionTableModel : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void trackTitleUpdateCausesMoveUpward();
    void trackTitleUpdateCausesNoMove_v1();
    void trackTitleUpdateCausesNoMove_v2();
    void trackTitleUpdateCausesMoveDownward();
    void trackTitleUpdateCausesMoveToLastPosition();
};

#endif
