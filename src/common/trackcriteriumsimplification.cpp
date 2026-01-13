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

#include "trackcriteriumsimplification.h"

namespace PMP
{
    std::unique_ptr<TrackCriterium> TrackCriteriumSimplifier::simplify(
        const TrackCriterium& originalCriterium)
    {
        TrackCriteriumSimplifier simplifier;
        originalCriterium.accept(simplifier);
        return std::move(simplifier._result);
    }

    void TrackCriteriumSimplifier::visit(const ConstantTrackCriterium& criterium)
    {
        _result = criterium.clone();
    }

    void TrackCriteriumSimplifier::visit(const TrackLengthPresenceCriterium& criterium)
    {
        _result = criterium.clone();
    }

    void TrackCriteriumSimplifier::visit(const TrackLengthComparisonCriterium& criterium)
    {
        _result = criterium.clone();
    }

    void TrackCriteriumSimplifier::visit(const TrackScorePresenceCriterium& criterium)
    {
        _result = criterium.clone();
    }

    void TrackCriteriumSimplifier::visit(const TrackScoreComparisonCriterium& criterium)
    {
        _result = criterium.clone();
    }

    void TrackCriteriumSimplifier::visit(const TrackLastHeardPresenceCriterium& criterium)
    {
        _result = criterium.clone();
    }

    void TrackCriteriumSimplifier::visit(const TrackLastHeardRecentlyCriterium& criterium)
    {
        _result = criterium.clone();
    }

    void TrackCriteriumSimplifier::visit(const TrackQueuePresenceCriterium& criterium)
    {
        _result = criterium.clone();
    }

    void TrackCriteriumSimplifier::visit(const TrackAvailabilityCriterium& criterium)
    {
        _result = criterium.clone();
    }

    void TrackCriteriumSimplifier::visit(const TrackMetaDataPresenceCriterium& criterium)
    {
        _result = criterium.clone();
    }

    void TrackCriteriumSimplifier::visit(const CompositeTrackCriterium& criterium)
    {
        std::vector<std::unique_ptr<TrackCriterium>> simplifiedMembers;

        // TODO : flatten nested composites

        for (auto& member : criterium.criteria())
        {
            member->accept(*this);
            auto simplifiedMember = std::move(_result);

            if (auto constant =
                dynamic_cast<ConstantTrackCriterium*>(simplifiedMember.get()))
            {
                if (constant->value() == true)
                    continue; // true constant can be dropped

                // false constant short-circuits the whole thing
                _result = ConstantTrackCriterium::noTracksMatch();
                return;
            }

            simplifiedMembers.push_back(std::move(simplifiedMember));
        }

        if (simplifiedMembers.empty())
        {
            _result = ConstantTrackCriterium::allTracksMatch();
            return;
        }

        if (simplifiedMembers.size() == 1)
        {
            _result = std::move(simplifiedMembers.front());
            return;
        }

        auto composite = std::make_unique<CompositeTrackCriterium>();
        for (auto& simplifiedMember : simplifiedMembers)
            composite->add(std::move(simplifiedMember));

        _result = std::move(composite);
    }
}
