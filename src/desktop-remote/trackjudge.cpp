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

#include <algorithm>

using namespace PMP::Client;

namespace PMP
{
    void TrackJudge::setUserId(quint32 userId)
    {
        _userId = userId;
        _haveUserId = true;

        _userDataFetcher.enableAutoFetchForUser(userId);
    }

    bool TrackJudge::setCriteria(QList<TrackCriterium> criteria)
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
                [](auto const& c) { return c == TrackCriterium::AllTracks; }
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

    QList<TrackCriterium> TrackJudge::simplifyCriteria(QList<TrackCriterium> criteria)
    {
        // we do only the most basic simplification for now

        QList<TrackCriterium> result;

        for (auto const& criterium : criteria)
        {
            if (criterium == TrackCriterium::AllTracks)
                continue; // no need to add it

            if (criterium == TrackCriterium::NoTracks)
            {
                // no tracks can match
                return { TrackCriterium::NoTracks };
            }

            result.append(criterium);
        }

        return result;
    }

    bool TrackJudge::usesUserData(TrackCriterium criterium)
    {
        switch (criterium)
        {
            case TrackCriterium::NeverHeard:
            case TrackCriterium::NotHeardInLast5Years:
            case TrackCriterium::NotHeardInLast3Years:
            case TrackCriterium::NotHeardInLast2Years:
            case TrackCriterium::NotHeardInLastYear:
            case TrackCriterium::NotHeardInLast180Days:
            case TrackCriterium::NotHeardInLast90Days:
            case TrackCriterium::NotHeardInLast30Days:
            case TrackCriterium::NotHeardInLast10Days:
            case TrackCriterium::HeardAtLeastOnce:
            case TrackCriterium::WithoutScore:
            case TrackCriterium::WithScore:
            case TrackCriterium::ScoreLessThan30:
            case TrackCriterium::ScoreLessThan50:
            case TrackCriterium::ScoreAtLeast80:
            case TrackCriterium::ScoreAtLeast85:
            case TrackCriterium::ScoreAtLeast90:
            case TrackCriterium::ScoreAtLeast95:
                return true;

            case TrackCriterium::AllTracks:
            case TrackCriterium::NoTracks:
            case TrackCriterium::LengthLessThanOneMinute:
            case TrackCriterium::LengthAtLeastOneMinute:
            case TrackCriterium::LengthLessThanTwoMinutes:
            case TrackCriterium::LengthAtLeastTwoMinutes:
            case TrackCriterium::LengthLessThanThreeMinutes:
            case TrackCriterium::LengthAtLeastThreeMinutes:
            case TrackCriterium::LengthLessThanFourMinutes:
            case TrackCriterium::LengthAtLeastFourMinutes:
            case TrackCriterium::LengthLessThanFiveMinutes:
            case TrackCriterium::LengthAtLeastFiveMinutes:
            case TrackCriterium::NotInTheQueue:
            case TrackCriterium::InTheQueue:
            case TrackCriterium::WithoutTitle:
            case TrackCriterium::WithoutArtist:
            case TrackCriterium::WithoutAlbum:
            case TrackCriterium::NoLongerAvailable:
                break;
        }

        return false;
    }

    bool TrackJudge::isTextFieldEmpty(QString contents)
    {
        return contents.trimmed().isEmpty();
    }

    TriBool TrackJudge::trackSatisfiesCriterium(const CollectionTrackInfo& track,
                                                TrackCriterium criterium) const
    {
        switch (criterium)
        {
            case TrackCriterium::AllTracks:
                return true;

            case TrackCriterium::NoTracks:
                return false;

            case TrackCriterium::NeverHeard:
            {
                auto evaluator = [](QDateTime prevHeard) { return !prevHeard.isValid(); };
                return trackSatisfiesLastHeardDateCriterium(track, evaluator);
            }
            case TrackCriterium::NotHeardInLast5Years:
                return trackSatisfiesNotHeardInTheLastXYearsCriterium(track, 5);

            case TrackCriterium::NotHeardInLast3Years:
                return trackSatisfiesNotHeardInTheLastXYearsCriterium(track, 3);

            case TrackCriterium::NotHeardInLast2Years:
                return trackSatisfiesNotHeardInTheLastXYearsCriterium(track, 2);

            case TrackCriterium::NotHeardInLastYear:
                return trackSatisfiesNotHeardInTheLastXYearsCriterium(track, 1);

            case TrackCriterium::NotHeardInLast180Days:
                return trackSatisfiesNotHeardInTheLastXDaysCriterium(track, 180);

            case TrackCriterium::NotHeardInLast90Days:
                return trackSatisfiesNotHeardInTheLastXDaysCriterium(track, 90);

            case TrackCriterium::NotHeardInLast30Days:
                return trackSatisfiesNotHeardInTheLastXDaysCriterium(track, 30);

            case TrackCriterium::NotHeardInLast10Days:
                return trackSatisfiesNotHeardInTheLastXDaysCriterium(track, 10);

            case TrackCriterium::HeardAtLeastOnce:
            {
                auto evaluator = [](QDateTime prevHeard) { return prevHeard.isValid(); };
                return trackSatisfiesLastHeardDateCriterium(track, evaluator);
            }
            case TrackCriterium::WithoutScore:
            {
                auto evaluator = [](int permillage) { return permillage < 0; };
                return trackSatisfiesScoreCriterium(track, evaluator);
            }
            case TrackCriterium::WithScore:
            {
                auto evaluator = [](int permillage) { return permillage >= 0; };
                return trackSatisfiesScoreCriterium(track, evaluator);
            }
            case TrackCriterium::ScoreLessThan30:
            {
                auto evaluator =
                    [](int permillage) { return permillage >= 0 && permillage < 300; };
                return trackSatisfiesScoreCriterium(track, evaluator);
            }
            case TrackCriterium::ScoreLessThan50:
            {
                auto evaluator =
                    [](int permillage) { return permillage >= 0 && permillage < 500; };
                return trackSatisfiesScoreCriterium(track, evaluator);
            }
            case TrackCriterium::ScoreAtLeast80:
            {
                auto evaluator = [](int permillage) { return permillage >= 800; };
                return trackSatisfiesScoreCriterium(track, evaluator);
            }
            case TrackCriterium::ScoreAtLeast85:
            {
                auto evaluator = [](int permillage) { return permillage >= 850; };
                return trackSatisfiesScoreCriterium(track, evaluator);
            }
            case TrackCriterium::ScoreAtLeast90:
            {
                auto evaluator = [](int permillage) { return permillage >= 900; };
                return trackSatisfiesScoreCriterium(track, evaluator);
            }
            case TrackCriterium::ScoreAtLeast95:
            {
                auto evaluator = [](int permillage) { return permillage >= 950; };
                return trackSatisfiesScoreCriterium(track, evaluator);
            }
            case TrackCriterium::LengthLessThanOneMinute:
                return trackLengthLessThanXMinutes(track, 1);

            case TrackCriterium::LengthAtLeastOneMinute:
                return trackLengthAtLeastXMinutes(track, 1);

            case TrackCriterium::LengthLessThanTwoMinutes:
                return trackLengthLessThanXMinutes(track, 2);

            case TrackCriterium::LengthAtLeastTwoMinutes:
                return trackLengthAtLeastXMinutes(track, 2);

            case TrackCriterium::LengthLessThanThreeMinutes:
                return trackLengthLessThanXMinutes(track, 3);

            case TrackCriterium::LengthAtLeastThreeMinutes:
                return trackLengthAtLeastXMinutes(track, 3);

            case TrackCriterium::LengthLessThanFourMinutes:
                return trackLengthLessThanXMinutes(track, 4);

            case TrackCriterium::LengthAtLeastFourMinutes:
                return trackLengthAtLeastXMinutes(track, 4);

            case TrackCriterium::LengthLessThanFiveMinutes:
                return trackLengthLessThanXMinutes(track, 5);

            case TrackCriterium::LengthAtLeastFiveMinutes:
                return trackLengthAtLeastXMinutes(track, 5);

            case TrackCriterium::NotInTheQueue:
                return !_queueHashesMonitor.isPresentInQueue(track.hashId());

            case TrackCriterium::InTheQueue:
                return _queueHashesMonitor.isPresentInQueue(track.hashId());

            case TrackCriterium::WithoutTitle:
                return isTextFieldEmpty(track.title());

            case TrackCriterium::WithoutArtist:
                return isTextFieldEmpty(track.artist());

            case TrackCriterium::WithoutAlbum:
                return isTextFieldEmpty(track.album());

            case TrackCriterium::NoLongerAvailable:
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
