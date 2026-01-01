/*
    Copyright (C) 2025, Kevin André <hyperquantum@gmail.com>

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

#ifndef PMP_COMMON_TRACKCRITERIUMEVALUATION_H
#define PMP_COMMON_TRACKCRITERIUMEVALUATION_H

#include "trackcriteria.h"

#include "common/nullable.h"
#include "common/tribool.h"

#include <QDateTime>

namespace PMP
{
    class TrackCriteriumEvaluationContext
    {
    public:
        virtual ~TrackCriteriumEvaluationContext() = default;

        virtual QDateTime currentDateTimeUtc() const = 0;

        virtual Nullable<int> lengthInMilliseconds() const = 0;
        virtual Nullable<QString> title() const = 0;
        virtual Nullable<QString> artist() const = 0;
        virtual Nullable<QString> album() const = 0;

        virtual bool isUserDataAvailable() const = 0;
        virtual Nullable<short> scorePermillage() const = 0;
        virtual Nullable<QDateTime> lastHeard() const = 0;

        virtual bool isAvailable() const = 0;
        virtual bool isPresentInQueue() const = 0;
    };

    class TrackCriteriumEvaluator final : public TrackCriteriumVisitor
    {
    public:
        static TriBool evaluate(const TrackCriterium& criterium,
                                const TrackCriteriumEvaluationContext& context);

        void visit(const ConstantTrackCriterium&) override;
        void visit(const TrackLengthPresenceCriterium&) override;
        void visit(const TrackLengthComparisonCriterium&) override;
        void visit(const TrackScorePresenceCriterium&) override;
        void visit(const TrackScoreComparisonCriterium&) override;
        void visit(const TrackLastHeardPresenceCriterium&) override;
        void visit(const TrackLastHeardRecentlyCriterium&) override;
        void visit(const TrackQueuePresenceCriterium&) override;
        void visit(const TrackAvailabilityCriterium&) override;
        void visit(const TrackMetaDataPresenceCriterium&) override;
        void visit(const CompositeTrackCriterium&) override;

    private:
        TrackCriteriumEvaluator(const TrackCriteriumEvaluationContext& context);

        TrackCriteriumEvaluationContext const& _context;
        TriBool _result;
    };
}
#endif
