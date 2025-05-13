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

#include "test_searchrank.h"

#include "client/searchrank.h"

#include <QtTest/QTest>

using namespace PMP::Client;

void TestSearchRank::exactMatchBetterThanNoMatch()
{
    SearchQuery query { "globe" };

    QVERIFY(SearchRank::isMatch(query, "globe"));
    QVERIFY(!SearchRank::isMatch(query, "plane"));

    QCOMPARE(SearchRank::compareMatches(query, "globe", "plane"), -1);
    QCOMPARE(SearchRank::compareMatches(query, "plane", "globe"), 1);
    QVERIFY(SearchRank::isBetterMatchThan(query, "globe", "plane"));
}

void TestSearchRank::longerMatchIsBetter()
{
    SearchQuery query { "abc def" };

    QVERIFY(SearchRank::isMatch(query, "abcdef123"));
    QVERIFY(SearchRank::isMatch(query, "abc12345def"));
    QVERIFY(SearchRank::isMatch(query, "defabc123"));

    QVERIFY(SearchRank::isBetterMatchThan(query, "abcdef123", "abc12345def"));
    QVERIFY(SearchRank::isBetterMatchThan(query, "abcdef123", "defabc123"));
}

void TestSearchRank::matchingSequentialPartsWithSpaceInBetweenTreatedAsLongerMatch()
{
    SearchQuery query { "abc def" };

    QVERIFY(SearchRank::isMatch(query, "abc def"));
    QVERIFY(SearchRank::isMatch(query, "abc12345def"));
    QVERIFY(SearchRank::isMatch(query, "defabc123"));

    QVERIFY(SearchRank::isBetterMatchThan(query, "abc def", "abc12345def"));
    QVERIFY(SearchRank::isBetterMatchThan(query, "abc def", "defabc123"));
}

void TestSearchRank::sequentialWordsRankHigherThanFurtherApart()
{
    SearchQuery query { "can you" };

    QVERIFY(SearchRank::isMatch(query, "Can You"));
    QVERIFY(SearchRank::isMatch(query, "Can Get You"));

    QVERIFY(SearchRank::isBetterMatchThan(query, "Can You", "Can Get You"));
}

void TestSearchRank::avoidCountingSameMatchMoreThanOnce()
{
    SearchQuery query { "dada" };

    QVERIFY(SearchRank::isMatch(query, "odadao"));
    QVERIFY(SearchRank::isMatch(query, "odadadao"));

    QCOMPARE(SearchRank::compareMatches(query, "odadao", "odadadao"), 0);
}

void TestSearchRank::twoWordsAreBetterMatchForDuplicateSearchWord()
{
    SearchQuery query { "down down" };

    QVERIFY(SearchRank::isMatch(query, "down down"));
    QVERIFY(SearchRank::isMatch(query, "down"));
    QVERIFY(SearchRank::isMatch(query, "upside down"));
    QVERIFY(SearchRank::isMatch(query, "down under"));

    QVERIFY(SearchRank::isBetterMatchThan(query, "down down", "down"));
    QVERIFY(SearchRank::isBetterMatchThan(query, "down down", "upside down"));
    QVERIFY(SearchRank::isBetterMatchThan(query, "down down", "down under"));
}

void TestSearchRank::matchAtStartOfWordIsBetterThanInTheMiddle()
{
    SearchQuery query { "form" };

    QVERIFY(SearchRank::isMatch(query, "Former"));
    QVERIFY(SearchRank::isMatch(query, "Informer"));

    QVERIFY(SearchRank::isBetterMatchThan(query, "Former", "Informer"));
}

void TestSearchRank::matchOfFullWordIsBetterThanPartOfWord()
{
    SearchQuery stepQuery { "step" };

    QVERIFY(SearchRank::isMatch(stepQuery, "Step"));
    QVERIFY(SearchRank::isMatch(stepQuery, "Steps"));
    QVERIFY(SearchRank::isMatch(stepQuery, "Stephen"));

    QVERIFY(SearchRank::isBetterMatchThan(stepQuery, "Step", "Steps"));
    QVERIFY(SearchRank::isBetterMatchThan(stepQuery, "Step", "Stephen"));

    SearchQuery stepsQuery { "steps" };

    QVERIFY(SearchRank::isMatch(stepsQuery, "Steps"));
    QVERIFY(SearchRank::isMatch(stepsQuery, "Footsteps"));

    QVERIFY(SearchRank::isBetterMatchThan(stepsQuery, "Steps", "Footsteps"));
}

QTEST_MAIN(TestSearchRank)
