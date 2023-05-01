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
                if (!track.lengthIsKnown()) return TriBool::unknown;
                return track.lengthInMilliseconds() < 60 * 1000;

            case TrackCriterium::LengthAtLeastFiveMinutes:
                if (!track.lengthIsKnown()) return TriBool::unknown;
                return track.lengthInMilliseconds() >= 5 * 60 * 1000;

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
}
