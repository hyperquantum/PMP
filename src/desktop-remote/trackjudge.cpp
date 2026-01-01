/*
    Copyright (C) 2016-2026, Kevin André <hyperquantum@gmail.com>

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

#include "trackjudge.h"

#include "common/trackcriteriumevaluation.h"
#include "common/trackcriteriumsimplification.h"

#include "client/collectiontrackinfo.h"
#include "client/queuehashesmonitor.h"
#include "client/userdatafetcher.h"

using namespace PMP::Client;

namespace PMP
{
    class TrackJudge::EvaluationContext final : public TrackCriteriumEvaluationContext
    {
    public:
        EvaluationContext(CollectionTrackInfo const& track,
                          UserDataFetcher& userDataFetcher,
                          QueueHashesMonitor& queueHashesMonitor,
                          quint32 userId, bool _haveUserId);

        QDateTime currentDateTimeUtc() const override;

        Nullable<int> lengthInMilliseconds() const override;
        Nullable<QString> title() const override;
        Nullable<QString> artist() const override;
        Nullable<QString> album() const override;

        bool isUserDataAvailable() const override;
        Nullable<short> scorePermillage() const override;
        Nullable<QDateTime> lastHeard() const override;

        bool isAvailable() const override;
        bool isPresentInQueue() const override;

    private:
        const CollectionTrackInfo& _track;
        UserDataFetcher& _userDataFetcher;
        QueueHashesMonitor& _queueHashesMonitor;
        quint32 _userId;
        bool _haveUserId;
    };

    TrackJudge::EvaluationContext::EvaluationContext(const CollectionTrackInfo& track,
                                                     UserDataFetcher& userDataFetcher,
                                                QueueHashesMonitor& queueHashesMonitor,
                                                     quint32 userId, bool _haveUserId)
     : _track(track),
        _userDataFetcher(userDataFetcher),
        _queueHashesMonitor(queueHashesMonitor),
        _userId(userId),
        _haveUserId(_haveUserId)
    {
        //
    }

    QDateTime TrackJudge::EvaluationContext::currentDateTimeUtc() const
    {
        return QDateTime::currentDateTimeUtc();
    }

    Nullable<int> TrackJudge::EvaluationContext::lengthInMilliseconds() const
    {
        if (!_track.lengthIsKnown())
            return null;

        return _track.lengthInMilliseconds();
    }

    Nullable<QString> TrackJudge::EvaluationContext::title() const
    {
        return _track.title();
    }

    Nullable<QString> TrackJudge::EvaluationContext::artist() const
    {
        return _track.artist();
    }

    Nullable<QString> TrackJudge::EvaluationContext::album() const
    {
        return _track.album();
    }

    bool TrackJudge::EvaluationContext::isUserDataAvailable() const
    {
        if (!_haveUserId)
            return false;

        return _userDataFetcher.checkHaveHashDataForUser(_userId, _track.hashId());
    }

    Nullable<short> TrackJudge::EvaluationContext::scorePermillage() const
    {
        auto userHashData = _userDataFetcher.getHashDataForUser(_userId, _track.hashId());
        if (userHashData == nullptr || userHashData->scoreReceived == false)
            return null; /* score unknown */

        if (userHashData->scorePermillage < 0)
            return null; /* track without score */

        return userHashData->scorePermillage;
    }

    Nullable<QDateTime> TrackJudge::EvaluationContext::lastHeard() const
    {
        auto userHashData = _userDataFetcher.getHashDataForUser(_userId, _track.hashId());
        if (userHashData == nullptr || userHashData->previouslyHeardReceived == false)
            return null; /* score unknown */

        if (userHashData->previouslyHeard.isValid() == false)
            return null;

        return userHashData->previouslyHeard;
    }

    bool TrackJudge::EvaluationContext::isAvailable() const
    {
        return _track.isAvailable();
    }

    bool TrackJudge::EvaluationContext::isPresentInQueue() const
    {
        return _queueHashesMonitor.isPresentInQueue(_track.hashId());
    }

    /* ============================================================================ */

    void TrackJudge::setUserId(quint32 userId)
    {
        _userId = userId;
        _haveUserId = true;

        _userDataFetcher.enableAutoFetchForUser(userId);
    }

    bool TrackJudge::setCriteria(QList<PredefinedTrackCriterium> criteria)
    {
        auto criteriumTree = convertToTrackCriterium(criteria);
        auto simplifiedTree = TrackCriteriumSimplifier::simplify(*criteriumTree);

        // a naive comparison is OK, this is mostly for eliminating redundant assignments
        if (_criteriumTree->equals(*simplifiedTree))
            return false;

        _criteriumTree = std::move(simplifiedTree);

        _criteriumTreeMatchesAllTracks =
            _criteriumTree->equals(*ConstantTrackCriterium::allTracksMatch());

        return true;
    }

    bool TrackJudge::criteriumUsesUserData() const
    {
        return _criteriumTree->usesUserData();
    }

    bool TrackJudge::criteriumResultsInAllTracks() const
    {
        return _criteriumTreeMatchesAllTracks;
    }

    TriBool TrackJudge::trackSatisfiesCriteria(CollectionTrackInfo const& track) const
    {
        EvaluationContext context(track, _userDataFetcher, _queueHashesMonitor,
                                  _userId, _haveUserId);

        TriBool result = TrackCriteriumEvaluator::evaluate(*_criteriumTree, context);

        return result;
    }
}
