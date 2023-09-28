/*
    Copyright (C) 2018-2023, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_TESTSCROBBLER_H
#define PMP_TESTSCROBBLER_H

#include "server/scrobblingbackend.h"
#include "server/scrobblingdataprovider.h"
#include "server/trackinfoprovider.h"
#include "server/tracktoscrobble.h"

#include <QQueue>

#include <memory>

class BackendMock : public PMP::Server::ScrobblingBackend
{
    Q_OBJECT
public:
    BackendMock(bool requireAuthentication);

    PMP::SimpleFuture<PMP::Server::Result> authenticateWithCredentials(
                                                               QString usernameOrEmail,
                                                               QString password) override;

    void setTemporaryUnavailabilitiesToStageForScrobbles(int count);

    void setUserCredentials(QString username, QString password);
    void setApiToken(bool willBeAcceptedByApi);

    int nowPlayingUpdatedCount() const { return _nowPlayingUpdatedCount; }
    int scrobbledSuccessfullyCount() const { return _scrobbledSuccessfullyCount; }
    int tracksIgnoredCount() const { return _tracksIgnoredCount; }

public Q_SLOTS:
    void initialize() override;

    void updateNowPlaying(PMP::Server::ScrobblingTrack track) override;

    void scrobbleTrack(QDateTime timestamp,
                       PMP::Server::ScrobblingTrack track) override;

protected:
    bool needsSsl() const override { return false; }

private Q_SLOTS:
    void pretendAuthenticationResultReceived();
    void pretendSuccessfulNowPlaying();
    void pretendSuccessfulScrobble();
    void pretendScrobbleFailedBecauseTokenNoLongerValid();
    void pretendScrobbleFailedBecauseTrackIgnored();

private:
    int _nowPlayingUpdatedCount;
    int _temporaryUnavailabilitiesToStageAtScrobbleTime;
    int _scrobbledSuccessfullyCount;
    int _tracksIgnoredCount;
    QString _username;
    QString _password;
    bool _requireAuthentication;
    bool _haveApiToken;
    bool _apiTokenWillBeAcceptedByApi;
};

class TrackToScrobbleMock : public PMP::Server::TrackToScrobble
{
public:
    TrackToScrobbleMock(QDateTime timestamp, uint hashId);

    QDateTime timestamp() const override { return _timestamp; }
    uint hashId() const override { return _hashId; }

    void scrobbledSuccessfully() override;
    void scrobbleIgnored() override;

    bool scrobbled() const { return _scrobbled; }
    bool ignored() const { return _cannotBeScrobbled; }
    QDateTime scrobbledTimestamp() { return _scrobbledTimestamp; }

private:
    QDateTime _timestamp, _scrobbledTimestamp;
    uint _hashId;
    bool _scrobbled;
    bool _cannotBeScrobbled;
};

class DataProviderMock : public PMP::Server::ScrobblingDataProvider
{
public:
    DataProviderMock();

    void add(std::shared_ptr<PMP::Server::TrackToScrobble> track);
    void add(QVector<std::shared_ptr<TrackToScrobbleMock>> tracks);

    QVector<std::shared_ptr<PMP::Server::TrackToScrobble>> getNextTracksToScrobble() override;

private:
    QQueue<std::shared_ptr<PMP::Server::TrackToScrobble>> _tracksToScrobble;
};

class TrackInfoProviderMock : public PMP::Server::TrackInfoProvider
{
public:
    TrackInfoProviderMock();

    PMP::Future<PMP::Server::CollectionTrackInfo, PMP::FailureType> getTrackInfoAsync(
                                                                    uint hashId) override;

    void registerTrack(uint hashId, QString title, QString artist);
    void registerTrack(uint hashId, QString title, QString artist, QString album,
                       QString albumArtist);

private:
    QHash<uint, PMP::Server::CollectionTrackInfo> _tracks;
};

class TestScrobbler : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void simpleNowPlayingUpdate();
    void nowPlayingWithAuthentication();
    void nowPlayingWithTrackToScrobble();
    void nowPlayingWithImmediateScrobble();
    void trivialScrobble();
    void multipleSimpleScrobbles();
    void scrobbleWithAuthentication();
    void scrobbleWithExistingValidToken();
    void scrobbleWithTokenChangeAfterInvalidToken();
    void mustSkipScrobblesThatAreTooOld();
    void retriesAfterTemporaryUnavailability();

private:
    static PMP::Server::ScrobblingTrack createTrack();
    static QDateTime makeDateTime(int year, int month, int day, int hours, int minutes);
    std::shared_ptr<TrackToScrobbleMock> addTrackToScrobble(
                                                          DataProviderMock& dataProvider);
    std::shared_ptr<TrackToScrobbleMock> addTrackToScrobble(
                                                           DataProviderMock& dataProvider,
                                                           QDateTime time);
    std::shared_ptr<TrackToScrobbleMock> addTrackToScrobble(
                                                           DataProviderMock& dataProvider,
                                                TrackInfoProviderMock& trackInfoProvider,
                                                           QDateTime time, uint hashId,
                                                           QString title, QString artist);
};
#endif
