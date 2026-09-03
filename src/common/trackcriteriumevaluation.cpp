/*
    Copyright (C) 2025-2026, Kevin André <hyperquantum@gmail.com>

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

#include "trackcriteriumevaluation.h"

namespace PMP
{
    namespace
    {
        template<class T>
        bool evaluateComparison(T left, ComparisonOperator comparison, T right)
        {
            switch (comparison)
            {
            case ComparisonOperator::Equal: return left == right;
            case ComparisonOperator::NotEqual: return left != right;
            case ComparisonOperator::LessThan: return left < right;
            case ComparisonOperator::LessThanOrEqual: return left <= right;
            case ComparisonOperator::GreaterThan: return left > right;
            case ComparisonOperator::GreaterThanOrEqual: return left >= right;
            }

            Q_UNREACHABLE();
        }

        TriBool stringIsEmpty(Nullable<QString> s)
        {
            if (s.isNull())
                return TriBool::unknown;

            return s.value().trimmed().isEmpty();
        }

        TriBool stringIsNotEmpty(Nullable<QString> s)
        {
            if (s.isNull())
                return TriBool::unknown;

            return !s.value().trimmed().isEmpty();
        }
    }

    TriBool TrackCriteriumEvaluator::evaluate(const TrackCriterium& criterium,
                                           const TrackCriteriumEvaluationContext& context)
    {
        TrackCriteriumEvaluator evaluator { context };
        criterium.accept(evaluator);
        return evaluator._result;
    }

    void TrackCriteriumEvaluator::visit(const ConstantTrackCriterium& criterium)
    {
        _result = criterium.value();
    }

    void TrackCriteriumEvaluator::visit(const TrackLengthPresenceCriterium& criterium)
    {
        auto lengthMs = _context.lengthInMilliseconds();

        _result = lengthMs.hasValue() == criterium.presence();
    }

    void TrackCriteriumEvaluator::visit(const TrackLengthComparisonCriterium& criterium)
    {
        auto trackLengthMs = _context.lengthInMilliseconds();

        if (trackLengthMs.isNull())
        {
            _result = TriBool::unknown;
            return;
        }

        auto op = criterium.comparisonOperator();
        auto criteriumMs =
            criterium.hours() * 60 * 60 * 1000
            + criterium.minutes() * 60 * 1000
            + criterium.seconds() * 1000;

        _result = evaluateComparison(trackLengthMs.value(), op, criteriumMs);
    }

    void TrackCriteriumEvaluator::visit(const TrackScorePresenceCriterium& criterium)
    {
        if (_context.isUserDataAvailable() == false)
        {
            _result = TriBool::unknown;
        }
        else
        {
            auto permillage = _context.scorePermillage();
            _result = permillage.hasValue() == criterium.presence();
        }
    }

    void TrackCriteriumEvaluator::visit(const TrackScoreComparisonCriterium& criterium)
    {
        if (_context.isUserDataAvailable() == false)
        {
            _result = TriBool::unknown;
            return;
        }

        auto trackPermillage = _context.scorePermillage();
        if (trackPermillage.isNull()) // if track without score
        {
            _result = false;
            return;
        }

        auto op = criterium.comparisonOperator();
        auto criteriumPermillage = criterium.scorePermillage();

        _result = evaluateComparison(trackPermillage.value(), op, criteriumPermillage);
    }

    void TrackCriteriumEvaluator::visit(const TrackLastHeardPresenceCriterium& criterium)
    {
        if (_context.isUserDataAvailable() == false)
        {
            _result = TriBool::unknown;
        }
        else
        {
            auto lastHeard = _context.lastHeard();
            _result = lastHeard.hasValue() == criterium.presence();
        }
    }

    void TrackCriteriumEvaluator::visit(const TrackLastHeardRecentlyCriterium& criterium)
    {
        if (_context.isUserDataAvailable() == false)
        {
            _result = TriBool::unknown;
            return;
        }

        auto trackLastHeard = _context.lastHeard();
        if (trackLastHeard.isNull()) // if track without last heard (never played)
        {
            _result = criterium.isInverted();
            return;
        }

        auto duration = criterium.duration();
        auto startOfDuration =
            _context.currentDateTimeUtc()
                .addYears(-duration.years)
                .addDays(-duration.days)
                .addSecs(-duration.hours * 60 * 60);

        _result = criterium.isInverted()
                    ? trackLastHeard.value() <= startOfDuration
                    : trackLastHeard.value() > startOfDuration;
    }

    void TrackCriteriumEvaluator::visit(const TrackQueuePresenceCriterium& criterium)
    {
        _result = _context.isPresentInQueue() == criterium.presence();
    }

    void TrackCriteriumEvaluator::visit(const TrackAvailabilityCriterium& criterium)
    {
        _result = _context.isAvailable() == criterium.availability();
    }

    void TrackCriteriumEvaluator::visit(const TrackMetaDataPresenceCriterium& criterium)
    {
        switch (criterium.metaDataKind())
        {
        case TrackMetaDataKind::Title:
            _result =
                criterium.presence()
                          ? stringIsNotEmpty(_context.title())
                          : stringIsEmpty(_context.title());
            return;

        case TrackMetaDataKind::Artist:
            _result =
                criterium.presence()
                    ? stringIsNotEmpty(_context.artist())
                    : stringIsEmpty(_context.artist());
            return;

        case TrackMetaDataKind::Album:
            _result =
                criterium.presence()
                    ? stringIsNotEmpty(_context.album())
                    : stringIsEmpty(_context.album());
            return;
        }

        Q_UNREACHABLE();
    }

    void TrackCriteriumEvaluator::visit(const TrackLabelPresenceCriterium& criterium)
    {
        auto hasThatLabel = _context.hasLabel(criterium.labelId());

        _result = criterium.presence() ? hasThatLabel : !hasThatLabel;
    }

    void TrackCriteriumEvaluator::visit(const CompositeTrackCriterium& criterium)
    {
        auto const& memberCriteria = criterium.criteria();

        TriBool compositeResult = true; // without members the result would be true

        for (auto& memberCriterium : memberCriteria)
        {
            memberCriterium->accept(*this);

            compositeResult &= _result;

            if (compositeResult.isFalse())
                break; // short circuit
        }

        _result = compositeResult;
    }

    TrackCriteriumEvaluator::TrackCriteriumEvaluator(
        const TrackCriteriumEvaluationContext& context)
     : _context(context)
    {
        //
    }
}
