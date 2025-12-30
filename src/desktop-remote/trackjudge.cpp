/*
    Copyright (C) 2016-2025, Kevin André <hyperquantum@gmail.com>

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

#include "client/collectiontrackinfo.h"
#include "client/queuehashesmonitor.h"
#include "client/userdatafetcher.h"

#include "trackcriteriumevaluation.h"

#include <algorithm>

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
        auto simplified = simplifyCriteria(criteria);

        // a naive comparison is OK, this is mostly for eliminating redundant assignments
        if (_criteria == simplified)
            return false;

        _criteria = criteria;
        return true;
    }

    bool TrackJudge::criteriumUsesUserData() const
    {
        if (_criteria.isEmpty())
            return false;

        return
            std::any_of(
                _criteria.constBegin(), _criteria.constEnd(),
                [](auto const& c) { return usesUserData(c); }
            );
    }

    bool TrackJudge::criteriumResultsInAllTracks() const
    {
        if (_criteria.isEmpty())
            return true;

        return
            std::all_of(
                _criteria.constBegin(), _criteria.constEnd(),
                [](auto const& c) { return c == PredefinedTrackCriterium::AllTracks; }
            );
    }

    TriBool TrackJudge::trackSatisfiesCriteria(CollectionTrackInfo const& track) const
    {
        TriBool result = true;

        for (auto const& criterium : _criteria)
        {
            auto criteriumResult = trackSatisfiesCriterium(track, criterium);

            if (criteriumResult.isFalse())
                return false;

            result &= criteriumResult;
        }

        return result;
    }

    QList<PredefinedTrackCriterium> TrackJudge::simplifyCriteria(QList<PredefinedTrackCriterium> criteria)
    {
        // we do only the most basic simplification for now

        QList<PredefinedTrackCriterium> result;

        for (auto const& criterium : criteria)
        {
            if (criterium == PredefinedTrackCriterium::AllTracks)
                continue; // no need to add it

            if (criterium == PredefinedTrackCriterium::NoTracks)
            {
                // no tracks can match
                return { PredefinedTrackCriterium::NoTracks };
            }

            result.append(criterium);
        }

        return result;
    }

    bool TrackJudge::usesUserData(PredefinedTrackCriterium criterium)
    {
        switch (criterium)
        {
            case PredefinedTrackCriterium::NeverHeard:
            case PredefinedTrackCriterium::NotHeardInLast5Years:
            case PredefinedTrackCriterium::NotHeardInLast3Years:
            case PredefinedTrackCriterium::NotHeardInLast2Years:
            case PredefinedTrackCriterium::NotHeardInLastYear:
            case PredefinedTrackCriterium::NotHeardInLast180Days:
            case PredefinedTrackCriterium::NotHeardInLast90Days:
            case PredefinedTrackCriterium::NotHeardInLast30Days:
            case PredefinedTrackCriterium::NotHeardInLast10Days:
            case PredefinedTrackCriterium::HeardAtLeastOnce:
            case PredefinedTrackCriterium::WithoutScore:
            case PredefinedTrackCriterium::WithScore:
            case PredefinedTrackCriterium::ScoreLessThan30:
            case PredefinedTrackCriterium::ScoreLessThan50:
            case PredefinedTrackCriterium::ScoreAtLeast80:
            case PredefinedTrackCriterium::ScoreAtLeast85:
            case PredefinedTrackCriterium::ScoreAtLeast90:
            case PredefinedTrackCriterium::ScoreAtLeast95:
                return true;

            case PredefinedTrackCriterium::AllTracks:
            case PredefinedTrackCriterium::NoTracks:
            case PredefinedTrackCriterium::LengthLessThanOneMinute:
            case PredefinedTrackCriterium::LengthAtLeastOneMinute:
            case PredefinedTrackCriterium::LengthLessThanTwoMinutes:
            case PredefinedTrackCriterium::LengthAtLeastTwoMinutes:
            case PredefinedTrackCriterium::LengthLessThanThreeMinutes:
            case PredefinedTrackCriterium::LengthAtLeastThreeMinutes:
            case PredefinedTrackCriterium::LengthLessThanFourMinutes:
            case PredefinedTrackCriterium::LengthAtLeastFourMinutes:
            case PredefinedTrackCriterium::LengthLessThanFiveMinutes:
            case PredefinedTrackCriterium::LengthAtLeastFiveMinutes:
            case PredefinedTrackCriterium::NotInTheQueue:
            case PredefinedTrackCriterium::InTheQueue:
            case PredefinedTrackCriterium::WithoutTitle:
            case PredefinedTrackCriterium::WithoutArtist:
            case PredefinedTrackCriterium::WithoutAlbum:
            case PredefinedTrackCriterium::NoLongerAvailable:
                break;
        }

        return false;
    }

    bool TrackJudge::isTextFieldEmpty(QString contents)
    {
        return contents.trimmed().isEmpty();
    }

    TriBool TrackJudge::trackSatisfiesCriterium(const CollectionTrackInfo& track,
                                                PredefinedTrackCriterium criterium) const
    {
        switch (criterium)
        {
            case PredefinedTrackCriterium::AllTracks:
                return true;

            case PredefinedTrackCriterium::NoTracks:
                return false;

            case PredefinedTrackCriterium::NeverHeard:
            {
                auto evaluator = [](QDateTime prevHeard) { return !prevHeard.isValid(); };
                return trackSatisfiesLastHeardDateCriterium(track, evaluator);
            }
            case PredefinedTrackCriterium::NotHeardInLast5Years:
                return trackSatisfiesNotHeardInTheLastXYearsCriterium(track, 5);

            case PredefinedTrackCriterium::NotHeardInLast3Years:
                return trackSatisfiesNotHeardInTheLastXYearsCriterium(track, 3);

            case PredefinedTrackCriterium::NotHeardInLast2Years:
                return trackSatisfiesNotHeardInTheLastXYearsCriterium(track, 2);

            case PredefinedTrackCriterium::NotHeardInLastYear:
                return trackSatisfiesNotHeardInTheLastXYearsCriterium(track, 1);

            case PredefinedTrackCriterium::NotHeardInLast180Days:
                return trackSatisfiesNotHeardInTheLastXDaysCriterium(track, 180);

            case PredefinedTrackCriterium::NotHeardInLast90Days:
                return trackSatisfiesNotHeardInTheLastXDaysCriterium(track, 90);

            case PredefinedTrackCriterium::NotHeardInLast30Days:
                return trackSatisfiesNotHeardInTheLastXDaysCriterium(track, 30);

            case PredefinedTrackCriterium::NotHeardInLast10Days:
                return trackSatisfiesNotHeardInTheLastXDaysCriterium(track, 10);

            case PredefinedTrackCriterium::HeardAtLeastOnce:
            {
                auto evaluator = [](QDateTime prevHeard) { return prevHeard.isValid(); };
                return trackSatisfiesLastHeardDateCriterium(track, evaluator);
            }
            case PredefinedTrackCriterium::WithoutScore:
            {
                auto evaluator = [](int permillage) { return permillage < 0; };
                return trackSatisfiesScoreCriterium(track, evaluator);
            }
            case PredefinedTrackCriterium::WithScore:
            {
                auto evaluator = [](int permillage) { return permillage >= 0; };
                return trackSatisfiesScoreCriterium(track, evaluator);
            }
            case PredefinedTrackCriterium::ScoreLessThan30:
            {
                auto evaluator =
                    [](int permillage) { return permillage >= 0 && permillage < 300; };
                return trackSatisfiesScoreCriterium(track, evaluator);
            }
            case PredefinedTrackCriterium::ScoreLessThan50:
            {
                auto evaluator =
                    [](int permillage) { return permillage >= 0 && permillage < 500; };
                return trackSatisfiesScoreCriterium(track, evaluator);
            }
            case PredefinedTrackCriterium::ScoreAtLeast80:
            {
                auto evaluator = [](int permillage) { return permillage >= 800; };
                return trackSatisfiesScoreCriterium(track, evaluator);
            }
            case PredefinedTrackCriterium::ScoreAtLeast85:
            {
                auto evaluator = [](int permillage) { return permillage >= 850; };
                return trackSatisfiesScoreCriterium(track, evaluator);
            }
            case PredefinedTrackCriterium::ScoreAtLeast90:
            {
                auto evaluator = [](int permillage) { return permillage >= 900; };
                return trackSatisfiesScoreCriterium(track, evaluator);
            }
            case PredefinedTrackCriterium::ScoreAtLeast95:
            {
                auto evaluator = [](int permillage) { return permillage >= 950; };
                return trackSatisfiesScoreCriterium(track, evaluator);
            }
            case PredefinedTrackCriterium::LengthLessThanOneMinute:
                return trackLengthLessThanXMinutes(track, 1);

            case PredefinedTrackCriterium::LengthAtLeastOneMinute:
                return trackLengthAtLeastXMinutes(track, 1);

            case PredefinedTrackCriterium::LengthLessThanTwoMinutes:
                return trackLengthLessThanXMinutes(track, 2);

            case PredefinedTrackCriterium::LengthAtLeastTwoMinutes:
                return trackLengthAtLeastXMinutes(track, 2);

            case PredefinedTrackCriterium::LengthLessThanThreeMinutes:
                return trackLengthLessThanXMinutes(track, 3);

            case PredefinedTrackCriterium::LengthAtLeastThreeMinutes:
                return trackLengthAtLeastXMinutes(track, 3);

            case PredefinedTrackCriterium::LengthLessThanFourMinutes:
                return trackLengthLessThanXMinutes(track, 4);

            case PredefinedTrackCriterium::LengthAtLeastFourMinutes:
                return trackLengthAtLeastXMinutes(track, 4);

            case PredefinedTrackCriterium::LengthLessThanFiveMinutes:
                return trackLengthLessThanXMinutes(track, 5);

            case PredefinedTrackCriterium::LengthAtLeastFiveMinutes:
                return trackLengthAtLeastXMinutes(track, 5);

            case PredefinedTrackCriterium::NotInTheQueue:
                return !_queueHashesMonitor.isPresentInQueue(track.hashId());

            case PredefinedTrackCriterium::InTheQueue:
                return _queueHashesMonitor.isPresentInQueue(track.hashId());

            case PredefinedTrackCriterium::WithoutTitle:
                return isTextFieldEmpty(track.title());

            case PredefinedTrackCriterium::WithoutArtist:
                return isTextFieldEmpty(track.artist());

            case PredefinedTrackCriterium::WithoutAlbum:
                return isTextFieldEmpty(track.album());

            case PredefinedTrackCriterium::NoLongerAvailable:
                return track.isAvailable() == false;
        }

        return false;
    }

    TriBool TrackJudge::trackSatisfiesScoreCriterium(
                               CollectionTrackInfo const& track,
                               std::function<TriBool(int)> scorePermillageEvaluator) const
    {
        if (!_haveUserId) return TriBool::unknown;

        auto hashDataForUser =
                _userDataFetcher.getHashDataForUser(_userId, track.hashId());

        if (!hashDataForUser || !hashDataForUser->scoreReceived)
            return TriBool::unknown;

        return scorePermillageEvaluator(hashDataForUser->scorePermillage);
    }

    TriBool TrackJudge::trackSatisfiesLastHeardDateCriterium(
                                    CollectionTrackInfo const& track,
                                    std::function<TriBool(QDateTime)> dateEvaluator) const
    {
        if (!_haveUserId) return TriBool::unknown;

        auto hashDataForUser =
                _userDataFetcher.getHashDataForUser(_userId, track.hashId());

        if (!hashDataForUser || !hashDataForUser->previouslyHeardReceived)
            return TriBool::unknown;

        return dateEvaluator(hashDataForUser->previouslyHeard);
    }

    TriBool TrackJudge::trackSatisfiesNotHeardInTheLastXDaysCriterium(
                                                         const CollectionTrackInfo& track,
                                                         int days) const
    {
        auto evaluator =
            [days](QDateTime prevHeard)
            {
                return !prevHeard.isValid()
                        || prevHeard <= QDateTime::currentDateTimeUtc().addDays(-days);
            };

        return trackSatisfiesLastHeardDateCriterium(track, evaluator);
    }

    TriBool TrackJudge::trackSatisfiesNotHeardInTheLastXYearsCriterium(
                                                         const CollectionTrackInfo& track,
                                                         int years) const
    {
        auto evaluator =
            [years](QDateTime prevHeard)
            {
                return !prevHeard.isValid()
                        || prevHeard <= QDateTime::currentDateTimeUtc().addYears(-years);
            };

        return trackSatisfiesLastHeardDateCriterium(track, evaluator);
    }

    TriBool TrackJudge::trackLengthLessThanXMinutes(
        const Client::CollectionTrackInfo& track, int minutes) const
    {
        if (!track.lengthIsKnown()) return TriBool::unknown;

        return track.lengthInMilliseconds() < minutes * 60 * 1000;
    }

    TriBool TrackJudge::trackLengthAtLeastXMinutes(
        const Client::CollectionTrackInfo& track, int minutes) const
    {
        if (!track.lengthIsKnown()) return TriBool::unknown;

        return track.lengthInMilliseconds() >= minutes * 60 * 1000;
    }
}
