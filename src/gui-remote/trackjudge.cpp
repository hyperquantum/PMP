/*
    Copyright (C) 2016-2023, Kevin Andre <hyperquantum@gmail.com>

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

using namespace PMP::Client;

namespace PMP
{
    void TrackJudge::setUserId(quint32 userId)
    {
        _userId = userId;
        _haveUserId = true;

        _userDataFetcher.enableAutoFetchForUser(userId);
    }

    bool TrackJudge::criteriumUsesUserData() const
    {
        return usesUserData(_criterium1) || usesUserData(_criterium2);
    }

    bool TrackJudge::criteriumResultsInAllTracks() const
    {
        return _criterium1 == TrackCriterium::AllTracks
                && _criterium2 == TrackCriterium::AllTracks;
    }

    TriBool TrackJudge::trackSatisfiesCriteria(CollectionTrackInfo const& track) const
    {
        auto satifiesCriterium1 = trackSatisfiesCriterium(track, _criterium1);
        if (satifiesCriterium1.isFalse())
            return false;

        auto satifiesCriterium2 = trackSatisfiesCriterium(track, _criterium2);
        return satifiesCriterium1 & satifiesCriterium2;
    }

    bool TrackJudge::usesUserData(TrackCriterium criterium)
    {
        switch (criterium)
        {
            case TrackCriterium::NeverHeard:
            case TrackCriterium::LastHeardNotInLast1000Days:
            case TrackCriterium::LastHeardNotInLast365Days:
            case TrackCriterium::LastHeardNotInLast180Days:
            case TrackCriterium::LastHeardNotInLast90Days:
            case TrackCriterium::LastHeardNotInLast30Days:
            case TrackCriterium::LastHeardNotInLast10Days:
            case TrackCriterium::WithoutScore:
            case TrackCriterium::ScoreMaximum30:
            case TrackCriterium::ScoreAtLeast85:
            case TrackCriterium::ScoreAtLeast90:
            case TrackCriterium::ScoreAtLeast95:
                return true;

            case TrackCriterium::AllTracks:
            case TrackCriterium::NoTracks:
            case TrackCriterium::LengthMaximumOneMinute:
            case TrackCriterium::LengthAtLeastFiveMinutes:
            case TrackCriterium::NotInTheQueue:
            case TrackCriterium::InTheQueue:
            case TrackCriterium::NoLongerAvailable:
                break;
        }

        return false;
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
            case TrackCriterium::LastHeardNotInLast1000Days:
                return trackSatisfiesNotHeardInTheLastXDaysCriterium(track, 1000);

            case TrackCriterium::LastHeardNotInLast365Days:
                return trackSatisfiesNotHeardInTheLastXDaysCriterium(track, 365);

            case TrackCriterium::LastHeardNotInLast180Days:
                return trackSatisfiesNotHeardInTheLastXDaysCriterium(track, 180);

            case TrackCriterium::LastHeardNotInLast90Days:
                return trackSatisfiesNotHeardInTheLastXDaysCriterium(track, 90);

            case TrackCriterium::LastHeardNotInLast30Days:
                return trackSatisfiesNotHeardInTheLastXDaysCriterium(track, 30);

            case TrackCriterium::LastHeardNotInLast10Days:
                return trackSatisfiesNotHeardInTheLastXDaysCriterium(track, 10);

            case TrackCriterium::WithoutScore:
            {
                auto evaluator = [](int permillage) { return permillage < 0; };
                return trackSatisfiesScoreCriterium(track, evaluator);
            }
            case TrackCriterium::ScoreMaximum30:
            {
                auto evaluator =
                    [](int permillage) { return permillage >= 0 && permillage <= 300; };
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
            case TrackCriterium::LengthMaximumOneMinute:
                if (!track.lengthIsKnown()) return TriBool::unknown;
                return track.lengthInMilliseconds() <= 60 * 1000;

            case TrackCriterium::LengthAtLeastFiveMinutes:
                if (!track.lengthIsKnown()) return TriBool::unknown;
                return track.lengthInMilliseconds() >= 5 * 60 * 1000;

            case TrackCriterium::NotInTheQueue:
                return !_queueHashesMonitor.isPresentInQueue(track.hashId());

            case TrackCriterium::InTheQueue:
                return _queueHashesMonitor.isPresentInQueue(track.hashId());

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
}
