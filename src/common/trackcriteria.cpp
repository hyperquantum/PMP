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
