/*
    Copyright (C) 2023-2026, Kevin André <hyperquantum@gmail.com>

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

#include "trackcriteria.h"

#include <algorithm>

namespace PMP
{
    std::unique_ptr<TrackCriterium> convertToTrackCriterium(
        PredefinedTrackCriterium criterium)
    {
        switch (criterium)
        {
        case PredefinedTrackCriterium::AllTracks:
            return ConstantTrackCriterium::allTracksMatch();

        case PredefinedTrackCriterium::NoTracks:
            return ConstantTrackCriterium::noTracksMatch();

        case PredefinedTrackCriterium::NeverHeard:
            return TrackCriteriumFactory::neverHeard();

        case PredefinedTrackCriterium::NotHeardInLast5Years:
            return TrackCriteriumFactory::notRecentlyHeard({ .years = 5 });

        case PredefinedTrackCriterium::NotHeardInLast3Years:
            return TrackCriteriumFactory::notRecentlyHeard({ .years = 3 });

        case PredefinedTrackCriterium::NotHeardInLast2Years:
            return TrackCriteriumFactory::notRecentlyHeard({ .years = 2 });

        case PredefinedTrackCriterium::NotHeardInLastYear:
            return TrackCriteriumFactory::notRecentlyHeard({ .years = 1 });

        case PredefinedTrackCriterium::NotHeardInLast180Days:
            return TrackCriteriumFactory::notRecentlyHeard({ .days = 180 });

        case PredefinedTrackCriterium::NotHeardInLast90Days:
            return TrackCriteriumFactory::notRecentlyHeard({ .days = 90 });

        case PredefinedTrackCriterium::NotHeardInLast30Days:
            return TrackCriteriumFactory::notRecentlyHeard({ .days = 30 });

        case PredefinedTrackCriterium::NotHeardInLast10Days:
            return TrackCriteriumFactory::notRecentlyHeard({ .days = 10 });

        case PredefinedTrackCriterium::HeardAtLeastOnce:
            return TrackCriteriumFactory::heardAtLeastOnce();

        case PredefinedTrackCriterium::WithoutScore:
            return TrackCriteriumFactory::scoreMustBeAbsent();

        case PredefinedTrackCriterium::WithScore:
            return TrackCriteriumFactory::scoreMustBePresent();

        case PredefinedTrackCriterium::ScoreLessThan30:
            return TrackCriteriumFactory::scoreLessThanXPercent(30);

        case PredefinedTrackCriterium::ScoreLessThan50:
            return TrackCriteriumFactory::scoreLessThanXPercent(50);

        case PredefinedTrackCriterium::ScoreAtLeast80:
            return TrackCriteriumFactory::scoreAtLeastXPercent(80);

        case PredefinedTrackCriterium::ScoreAtLeast85:
            return TrackCriteriumFactory::scoreAtLeastXPercent(85);

        case PredefinedTrackCriterium::ScoreAtLeast90:
            return TrackCriteriumFactory::scoreAtLeastXPercent(90);

        case PredefinedTrackCriterium::ScoreAtLeast95:
            return TrackCriteriumFactory::scoreAtLeastXPercent(95);

        case PredefinedTrackCriterium::LengthLessThanOneMinute:
            return TrackCriteriumFactory::lengthLessThanXMinutes(1);

        case PredefinedTrackCriterium::LengthAtLeastOneMinute:
            return TrackCriteriumFactory::lengthAtLeastXMinutes(1);

        case PredefinedTrackCriterium::LengthLessThanTwoMinutes:
            return TrackCriteriumFactory::lengthLessThanXMinutes(2);

        case PredefinedTrackCriterium::LengthAtLeastTwoMinutes:
            return TrackCriteriumFactory::lengthAtLeastXMinutes(2);

        case PredefinedTrackCriterium::LengthLessThanThreeMinutes:
            return TrackCriteriumFactory::lengthLessThanXMinutes(3);

        case PredefinedTrackCriterium::LengthAtLeastThreeMinutes:
            return TrackCriteriumFactory::lengthAtLeastXMinutes(3);

        case PredefinedTrackCriterium::LengthLessThanFourMinutes:
            return TrackCriteriumFactory::lengthLessThanXMinutes(4);

        case PredefinedTrackCriterium::LengthAtLeastFourMinutes:
            return TrackCriteriumFactory::lengthAtLeastXMinutes(4);

        case PredefinedTrackCriterium::LengthLessThanFiveMinutes:
            return TrackCriteriumFactory::lengthLessThanXMinutes(5);

        case PredefinedTrackCriterium::LengthAtLeastFiveMinutes:
            return TrackCriteriumFactory::lengthAtLeastXMinutes(5);

        case PredefinedTrackCriterium::NotInTheQueue:
            return TrackCriteriumFactory::notInTheQueue();

        case PredefinedTrackCriterium::InTheQueue:
            return TrackCriteriumFactory::inTheQueue();

        case PredefinedTrackCriterium::WithoutTitle:
            return TrackCriteriumFactory::withoutTitle();

        case PredefinedTrackCriterium::WithoutArtist:
            return TrackCriteriumFactory::withoutArtist();

        case PredefinedTrackCriterium::WithoutAlbum:
            return TrackCriteriumFactory::withoutAlbum();

        case PredefinedTrackCriterium::NoLongerAvailable:
            return TrackCriteriumFactory::unavailable();
        }

        /* should be unreachable because we handled all enum values */
        Q_UNREACHABLE();
    }

    std::unique_ptr<TrackCriterium> convertToTrackCriterium(
        const QList<PredefinedTrackCriterium>& criteria)
    {
        if (criteria.isEmpty())
            return ConstantTrackCriterium::allTracksMatch();

        if (criteria.size() == 1)
            return convertToTrackCriterium(criteria.front());

        auto composite = std::make_unique<CompositeTrackCriterium>();

        for (PredefinedTrackCriterium c : criteria)
        {
            composite->add(convertToTrackCriterium(c));
        }

        return composite;
    }

    /* ============================================================================ */

    TrackLengthComparisonCriterium::TrackLengthComparisonCriterium()
     : _operator(ComparisonOperator::GreaterThanOrEqual),
        _hours(0), _minutes(0), _seconds(0)
    {
        //
    }

    TrackLengthComparisonCriterium::TrackLengthComparisonCriterium(
        ComparisonOperator comparisonOperator, int hours, int minutes, int seconds)
     : _operator(comparisonOperator),
        _hours(hours), _minutes(minutes), _seconds(seconds)
    {
        //
    }

    /* ============================================================================ */

    TrackScoreComparisonCriterium::TrackScoreComparisonCriterium()
     : _operator(ComparisonOperator::GreaterThanOrEqual),
        _score(0)
    {
        //
    }

    TrackScoreComparisonCriterium::TrackScoreComparisonCriterium(
        ComparisonOperator comparisonOperator, short score)
     : _operator(comparisonOperator),
        _score(score)
    {
        //
    }

    /* ============================================================================ */

    TrackLastHeardRecentlyCriterium::TrackLastHeardRecentlyCriterium()
     : _duration(),
        _inverted(false)
    {
        //
    }

    TrackLastHeardRecentlyCriterium::TrackLastHeardRecentlyCriterium(
        CompositeDuration duration, bool isInverted)
     : _duration(duration),
        _inverted(isInverted)
    {
        //
    }

    /* ============================================================================ */

    CompositeTrackCriterium::CompositeTrackCriterium()
    {
        //
    }

    bool CompositeTrackCriterium::usesUserData() const
    {
        if (_criteria.empty())
            return false;

        return
            std::any_of(
                _criteria.begin(), _criteria.end(),
                [](auto const& c) { return c->usesUserData(); }
            );
    }
}
