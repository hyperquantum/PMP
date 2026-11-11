/*
    Copyright (C) 2026, Kevin André <hyperquantum@gmail.com>

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

#include "trackcriteriuminqueries.h"

namespace PMP
{
    class TrackCriteriumInqueries::DependsOnUserDataVisitor : public TrackCriteriumVisitor
    {
    public:
        static bool dependsOnUserData(TrackCriterium const& criterium);

        void visit(const ConstantTrackCriterium&) override { _result = false; }
        void visit(const TrackLengthPresenceCriterium&) override { _result = false; }
        void visit(const TrackLengthComparisonCriterium&) override { _result = false; }
        void visit(const TrackScorePresenceCriterium&) override { _result = true; }
        void visit(const TrackScoreComparisonCriterium&) override { _result = true; }
        void visit(const TrackLastHeardPresenceCriterium&) override { _result = true; }
        void visit(const TrackLastHeardRecentlyCriterium&) override { _result = true; }
        void visit(const TrackQueuePresenceCriterium&) override { _result = false; }
        void visit(const TrackAvailabilityCriterium&) override { _result = false; }
        void visit(const TrackMetaDataPresenceCriterium&) override { _result = false; }
        void visit(const TrackLabelPresenceCriterium&) override { _result = false; }
        void visit(const CompositeTrackCriterium&) override;

    private:
        bool _result { false };
    };

    bool TrackCriteriumInqueries::DependsOnUserDataVisitor::dependsOnUserData(
        const TrackCriterium& criterium)
    {
        DependsOnUserDataVisitor visitor;
        criterium.accept(visitor);
        return visitor._result;
    }

    void TrackCriteriumInqueries::DependsOnUserDataVisitor::visit(
        const CompositeTrackCriterium& criterium)
    {
        auto const& memberCriteria = criterium.criteria();

        bool compositeResult = false;

        for (auto& memberCriterium : memberCriteria)
        {
            memberCriterium->accept(*this);

            compositeResult |= _result;

            if (compositeResult == true)
                break; // short circuit
        }

        _result = compositeResult;
    }

    /* ============================================================================ */

    class TrackCriteriumInqueries::DependsOnLabelVisitor : public TrackCriteriumVisitor
    {
    public:
        static bool dependsOnLabel(const TrackCriterium& criterium, quint32 labelId);

        void visit(const ConstantTrackCriterium&) override { _result = false; }
        void visit(const TrackLengthPresenceCriterium&) override { _result = false; }
        void visit(const TrackLengthComparisonCriterium&) override { _result = false; }
        void visit(const TrackScorePresenceCriterium&) override { _result = false; }
        void visit(const TrackScoreComparisonCriterium&) override { _result = false; }
        void visit(const TrackLastHeardPresenceCriterium&) override { _result = false; }
        void visit(const TrackLastHeardRecentlyCriterium&) override { _result = false; }
        void visit(const TrackQueuePresenceCriterium&) override { _result = false; }
        void visit(const TrackAvailabilityCriterium&) override { _result = false; }
        void visit(const TrackMetaDataPresenceCriterium&) override { _result = false; }
        void visit(const TrackLabelPresenceCriterium&) override;
        void visit(const CompositeTrackCriterium&) override;

    private:
        DependsOnLabelVisitor(quint32 labelId) : _labelId(labelId) {};

        quint32 _labelId;
        bool _result { false };
    };

    bool TrackCriteriumInqueries::DependsOnLabelVisitor::dependsOnLabel(
        const TrackCriterium& criterium, quint32 labelId)
    {
        DependsOnLabelVisitor visitor(labelId);
        criterium.accept(visitor);
        return visitor._result;
    }

    void TrackCriteriumInqueries::DependsOnLabelVisitor::visit(
        const TrackLabelPresenceCriterium& criterium)
    {
        _result = _labelId == criterium.labelId();
    }

    void TrackCriteriumInqueries::DependsOnLabelVisitor::visit(
        const CompositeTrackCriterium& criterium)
    {
        auto const& memberCriteria = criterium.criteria();

        bool compositeResult = false;

        for (auto& memberCriterium : memberCriteria)
        {
            memberCriterium->accept(*this);

            compositeResult |= _result;

            if (compositeResult == true)
                break; // short circuit
        }

        _result = compositeResult;
    }

    /* ============================================================================ */

    bool TrackCriteriumInqueries::dependsOnUserData(const TrackCriterium& criterium)
    {
        return DependsOnUserDataVisitor::dependsOnUserData(criterium);
    }

    bool TrackCriteriumInqueries::dependsOnLabel(const TrackCriterium& criterium,
                                                 quint32 labelId)
    {
        return DependsOnLabelVisitor::dependsOnLabel(criterium, labelId);
    }
}
