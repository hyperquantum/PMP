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

#include "test_trackcriteriumsimplification.h"

#include "common/trackcriteriumsimplification.h"

#include <QtTest/QTest>

using namespace PMP;

namespace
{
    std::unique_ptr<ConstantTrackCriterium> createAllTracks()
    {
        return ConstantTrackCriterium::allTracksMatch();
    }

    std::unique_ptr<ConstantTrackCriterium> createNoTracks()
    {
        return ConstantTrackCriterium::noTracksMatch();
    }

    std::unique_ptr<TrackScoreComparisonCriterium> createScoreAtLeast(int minimumScore)
    {
        return std::make_unique<TrackScoreComparisonCriterium>(
            ComparisonOperator::GreaterThanOrEqual, minimumScore
        );
    }

    std::unique_ptr<TrackLastHeardRecentlyCriterium> createNotHeardInXDays(int days)
    {
        return std::make_unique<TrackLastHeardRecentlyCriterium>(
            CompositeDuration { .days = days }, /* inverted: */ true
        );
    }
}

void TestTrackCriteriumSimplification::allTracksSimplifiedToAllTracks()
{
    auto input = createAllTracks();

    auto simplified = TrackCriteriumSimplifier::simplify(*input);

    auto expected = createAllTracks();

    QVERIFY(simplified->equals(*expected));
}

void TestTrackCriteriumSimplification::noTracksSimplifiedToNoTracks()
{
    auto input = createNoTracks();

    auto simplified = TrackCriteriumSimplifier::simplify(*input);

    auto expected = createNoTracks();

    QVERIFY(simplified->equals(*expected));
}

void TestTrackCriteriumSimplification::emptyCompositeSimplifiedToAllTracks()
{
    auto input = std::make_unique<CompositeTrackCriterium>();

    auto simplified = TrackCriteriumSimplifier::simplify(*input);

    auto expected = createAllTracks();

    QVERIFY(simplified->equals(*expected));
}

void TestTrackCriteriumSimplification::compositeOnlyConsistingOfMultipleAllTracksSimplifiedToAllTracks()
{
    auto input = std::make_unique<CompositeTrackCriterium>();
    input->add(createAllTracks());
    input->add(createAllTracks());
    input->add(createAllTracks());
    input->add(createAllTracks());

    auto simplified = TrackCriteriumSimplifier::simplify(*input);

    auto expected = createAllTracks();

    QVERIFY(simplified->equals(*expected));
}

void TestTrackCriteriumSimplification::compositeContainingOneNoTracksSimplifiedToNoTracks()
{
    auto input = std::make_unique<CompositeTrackCriterium>();
    input->add(createAllTracks());
    input->add(TrackQueuePresenceCriterium::mustBeAbsentInQueue());
    input->add(createNoTracks());
    input->add<TrackLengthComparisonCriterium>(ComparisonOperator::GreaterThanOrEqual, 3);

    auto simplified = TrackCriteriumSimplifier::simplify(*input);

    auto expected = createNoTracks();

    QVERIFY(simplified->equals(*expected));
}

void TestTrackCriteriumSimplification::compositeIsSimplifiedToItsOneMemberThatMatters()
{
    auto input = std::make_unique<CompositeTrackCriterium>();
    input->add(createAllTracks());
    input->add(createScoreAtLeast(80));
    input->add(createAllTracks());
    input->add(createAllTracks());

    auto simplified = TrackCriteriumSimplifier::simplify(*input);

    auto expected = createScoreAtLeast(80);

    QVERIFY(simplified->equals(*expected));
}

void TestTrackCriteriumSimplification::compositeIsSimplifiedByRemovingAllTracksMembers()
{
    auto input = std::make_unique<CompositeTrackCriterium>();
    input->add(createAllTracks());
    input->add(createScoreAtLeast(80));
    input->add(createAllTracks());
    input->add(createNotHeardInXDays(90));
    input->add(createAllTracks());
    input->add(createAllTracks());

    auto simplified = TrackCriteriumSimplifier::simplify(*input);

    auto expected = std::make_unique<CompositeTrackCriterium>();
    expected->add(createScoreAtLeast(80));
    expected->add(createNotHeardInXDays(90));

    QVERIFY(simplified->equals(*expected));
}

QTEST_MAIN(TestTrackCriteriumSimplification)
