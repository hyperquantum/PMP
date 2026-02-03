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
    namespace
    {
        std::unique_ptr<TrackCriterium> createLengthLessThanCriterium(
            int lengthMinutesCeiling)
        {
            return std::make_unique<TrackLengthComparisonCriterium>(
                ComparisonOperator::LessThan, 0, lengthMinutesCeiling, 0);
        }

        std::unique_ptr<TrackCriterium> createLengthAtLeastCriterium(
            int minimumLengthMinutes)
        {
            return std::make_unique<TrackLengthComparisonCriterium>(
                ComparisonOperator::GreaterThanOrEqual, 0, minimumLengthMinutes, 0);
        }

        std::unique_ptr<TrackCriterium> createScoreLessThanCriterium(short scoreCeiling)
        {
            return std::make_unique<TrackScoreComparisonCriterium>(
                ComparisonOperator::LessThan, scoreCeiling);
        }

        std::unique_ptr<TrackCriterium> createScoreAtLeastCriterium(short minimumScore)
        {
            return std::make_unique<TrackScoreComparisonCriterium>(
                ComparisonOperator::GreaterThanOrEqual, minimumScore);
        }

        std::unique_ptr<TrackCriterium> createNotRecentlyHeardCriterium(
            CompositeDuration duration)
        {
            return std::make_unique<TrackLastHeardRecentlyCriterium>(
                duration, /* isInverted: */ true);
        }
    }

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
            return TrackLastHeardPresenceCriterium::lastHeardMustBeAbsent();

        case PredefinedTrackCriterium::NotHeardInLast5Years:
            return createNotRecentlyHeardCriterium(CompositeDuration { .years = 5 });

        case PredefinedTrackCriterium::NotHeardInLast3Years:
            return createNotRecentlyHeardCriterium(CompositeDuration { .years = 3 });

        case PredefinedTrackCriterium::NotHeardInLast2Years:
            return createNotRecentlyHeardCriterium(CompositeDuration { .years = 2 });

        case PredefinedTrackCriterium::NotHeardInLastYear:
            return createNotRecentlyHeardCriterium(CompositeDuration { .years = 1 });

        case PredefinedTrackCriterium::NotHeardInLast180Days:
            return createNotRecentlyHeardCriterium(CompositeDuration { .days = 180 });

        case PredefinedTrackCriterium::NotHeardInLast90Days:
            return createNotRecentlyHeardCriterium(CompositeDuration { .days = 90 });

        case PredefinedTrackCriterium::NotHeardInLast30Days:
            return createNotRecentlyHeardCriterium(CompositeDuration { .days = 30 });

        case PredefinedTrackCriterium::NotHeardInLast10Days:
            return createNotRecentlyHeardCriterium(CompositeDuration { .days = 10 });

        case PredefinedTrackCriterium::HeardAtLeastOnce:
            return TrackLastHeardPresenceCriterium::lastHeardMustBePresent();

        case PredefinedTrackCriterium::WithoutScore:
            return TrackScorePresenceCriterium::scoreMustBeAbsent();

        case PredefinedTrackCriterium::WithScore:
            return TrackScorePresenceCriterium::scoreMustBePresent();

        case PredefinedTrackCriterium::ScoreLessThan30:
            return createScoreLessThanCriterium(30);

        case PredefinedTrackCriterium::ScoreLessThan50:
            return createScoreLessThanCriterium(50);

        case PredefinedTrackCriterium::ScoreAtLeast80:
            return createScoreAtLeastCriterium(80);

        case PredefinedTrackCriterium::ScoreAtLeast85:
            return createScoreAtLeastCriterium(85);

        case PredefinedTrackCriterium::ScoreAtLeast90:
            return createScoreAtLeastCriterium(90);

        case PredefinedTrackCriterium::ScoreAtLeast95:
            return createScoreAtLeastCriterium(95);

        case PredefinedTrackCriterium::LengthLessThanOneMinute:
            return createLengthLessThanCriterium(1);

        case PredefinedTrackCriterium::LengthAtLeastOneMinute:
            return createLengthAtLeastCriterium(1);

        case PredefinedTrackCriterium::LengthLessThanTwoMinutes:
            return createLengthLessThanCriterium(2);

        case PredefinedTrackCriterium::LengthAtLeastTwoMinutes:
            return createLengthAtLeastCriterium(2);

        case PredefinedTrackCriterium::LengthLessThanThreeMinutes:
            return createLengthLessThanCriterium(3);

        case PredefinedTrackCriterium::LengthAtLeastThreeMinutes:
            return createLengthAtLeastCriterium(3);

        case PredefinedTrackCriterium::LengthLessThanFourMinutes:
            return createLengthLessThanCriterium(4);

        case PredefinedTrackCriterium::LengthAtLeastFourMinutes:
            return createLengthAtLeastCriterium(4);

        case PredefinedTrackCriterium::LengthLessThanFiveMinutes:
            return createLengthLessThanCriterium(5);

        case PredefinedTrackCriterium::LengthAtLeastFiveMinutes:
            return createLengthAtLeastCriterium(5);

        case PredefinedTrackCriterium::NotInTheQueue:
            return TrackQueuePresenceCriterium::mustBeAbsentInQueue();

        case PredefinedTrackCriterium::InTheQueue:
            return TrackQueuePresenceCriterium::mustBePresentInQueue();

        case PredefinedTrackCriterium::WithoutTitle:
            return TrackMetaDataPresenceCriterium::mustBeAbsent(TrackMetaDataKind::Title);

        case PredefinedTrackCriterium::WithoutArtist:
            return TrackMetaDataPresenceCriterium::mustBeAbsent(
                TrackMetaDataKind::Artist);

        case PredefinedTrackCriterium::WithoutAlbum:
            return TrackMetaDataPresenceCriterium::mustBeAbsent(TrackMetaDataKind::Album);

        case PredefinedTrackCriterium::NoLongerAvailable:
            return TrackAvailabilityCriterium::mustBeUnavailable();
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
