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

#include "test_labels.h"

#include "server/labels.h"

#include <QtTest/QTest>

using namespace PMP::Server;

void TestLabels::isValidPotentialName_rejectsInvalidNames()
{
    QCOMPARE(Labels::isValidPotentialName(""), false);

    QCOMPARE(Labels::isValidPotentialName(" "), false);
    QCOMPARE(Labels::isValidPotentialName("-"), false);
    QCOMPARE(Labels::isValidPotentialName("_"), false);
    QCOMPARE(Labels::isValidPotentialName("+"), false);
    QCOMPARE(Labels::isValidPotentialName("!"), false);
    QCOMPARE(Labels::isValidPotentialName("?"), false);
    QCOMPARE(Labels::isValidPotentialName("#"), false);
    QCOMPARE(Labels::isValidPotentialName("@"), false);
    QCOMPARE(Labels::isValidPotentialName("("), false);
    QCOMPARE(Labels::isValidPotentialName(")"), false);

    QCOMPARE(Labels::isValidPotentialName("9+"), false);
    QCOMPARE(Labels::isValidPotentialName("9-"), false);
    QCOMPARE(Labels::isValidPotentialName("+9"), false);
    QCOMPARE(Labels::isValidPotentialName("-9"), false);
    QCOMPARE(Labels::isValidPotentialName("--"), false);

    QCOMPARE(Labels::isValidPotentialName("abc+"), false);
    QCOMPARE(Labels::isValidPotentialName("abc/"), false);
    QCOMPARE(Labels::isValidPotentialName(" abc"), false);
    QCOMPARE(Labels::isValidPotentialName("-abc"), false);
    QCOMPARE(Labels::isValidPotentialName("!abc"), false);

    QCOMPARE(Labels::isValidPotentialName("ab--c"), false);
    QCOMPARE(Labels::isValidPotentialName("90's"), false);

    QCOMPARE(Labels::isValidPotentialName("top 100"), false);
    QCOMPARE(Labels::isValidPotentialName("top_100"), false);

    QCOMPARE(Labels::isValidPotentialName("ABC"), false);
    QCOMPARE(Labels::isValidPotentialName("Abc"), false);
}

void TestLabels::isValidPotentialName_acceptsValidNames()
{
    QCOMPARE(Labels::isValidPotentialName("a"), true);
    QCOMPARE(Labels::isValidPotentialName("3"), true);
    QCOMPARE(Labels::isValidPotentialName("007"), true);
    QCOMPARE(Labels::isValidPotentialName("abc"), true);
    QCOMPARE(Labels::isValidPotentialName("90s"), true);
    QCOMPARE(Labels::isValidPotentialName("top-100"), true);
    QCOMPARE(Labels::isValidPotentialName("without-vocals"), true);
    QCOMPARE(Labels::isValidPotentialName("instrumental"), true);
    QCOMPARE(Labels::isValidPotentialName("not-for-weddings"), true);
}

QTEST_MAIN(TestLabels)
