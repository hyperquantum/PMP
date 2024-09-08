/*
    Copyright (C) 2024, Kevin André <hyperquantum@gmail.com>

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

#include "test_searchutil.h"

#include "common/searchutil.h"

#include <QtTest/QTest>

using namespace PMP;

QTEST_MAIN(TestSearchUtil)

void TestSearchUtil::toSearchString_preservesSpecialChars()
{
    QCOMPARE(SearchUtil::toSearchString("nice shoes!"), "nice shoes!");
    QCOMPARE(SearchUtil::toSearchString("n/a"), "n/a");
    QCOMPARE(SearchUtil::toSearchString("..."), "...");
}

void TestSearchUtil::toSearchString_convertsToLowercase()
{
    QCOMPARE(SearchUtil::toSearchString("TLC"), "tlc");
    QCOMPARE(SearchUtil::toSearchString("Milk Inc"), "milk inc");
    QCOMPARE(SearchUtil::toSearchString("aBBa"), "abba");
}

void TestSearchUtil::toSearchString_eliminatesLeadingSpaces()
{
    QCOMPARE(SearchUtil::toSearchString(" leading"), "leading");
}

void TestSearchUtil::toSearchString_eliminatesTrailingSpaces()
{
    QCOMPARE(SearchUtil::toSearchString("trailing "), "trailing");
}

void TestSearchUtil::toSearchString_eliminatesRedundantSpaces()
{
    QCOMPARE(SearchUtil::toSearchString("tell   me  lies"), "tell me lies");
    QCOMPARE(SearchUtil::toSearchString("                 "), "");
}

void TestSearchUtil::toSearchString_removesAccents()
{
    QCOMPARE(SearchUtil::toSearchString("café"), "cafe");
    QCOMPARE(SearchUtil::toSearchString("tiësto"), "tiesto");
}

void TestSearchUtil::toSearchString_testCombinations()
{
    QCOMPARE(SearchUtil::toSearchString("Tiësto"), "tiesto");
    QCOMPARE(SearchUtil::toSearchString("DJ  TIËSTO"), "dj tiesto");
    QCOMPARE(SearchUtil::toSearchString("Adam K & Soha Vocal Mix"),
                                        "adam k & soha vocal mix");
}
