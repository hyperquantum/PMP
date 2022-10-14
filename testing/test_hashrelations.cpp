/*
    Copyright (C) 2022, Kevin Andre <hyperquantum@gmail.com>

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

#include "test_hashrelations.h"

#include "server/hashrelations.h"

#include <QtTest/QTest>

using namespace PMP;

void TestHashRelations::areEquivalent()
{
    HashRelations r;
    QVERIFY(!r.areEquivalent({1, 2}));
    QVERIFY(!r.areEquivalent({1, 2, 3}));

    r.markAsEquivalent({1, 2});
    QVERIFY(r.areEquivalent({1, 2}));
    QVERIFY(!r.areEquivalent({1, 2, 3}));

    r.markAsEquivalent({1, 3});
    QVERIFY(r.areEquivalent({1, 2}));
    QVERIFY(r.areEquivalent({1, 2, 3}));

    r.markAsEquivalent({10, 11});
    QVERIFY(r.areEquivalent({11, 10}));
    QVERIFY(!r.areEquivalent({2, 10}));
}

void TestHashRelations::getEquivalencyGroup_groupIsTheSameForEachMember()
{
    HashRelations r;
    r.markAsEquivalent({2, 5, 9});

    auto g2 = r.getEquivalencyGroup(2);
    QCOMPARE(g2.size(), 3);
    QVERIFY(g2.contains(2));
    QVERIFY(g2.contains(5));
    QVERIFY(g2.contains(9));

    auto g5 = r.getEquivalencyGroup(2);
    QCOMPARE(g5.size(), 3);
    QVERIFY(g5.contains(2));
    QVERIFY(g5.contains(5));
    QVERIFY(g5.contains(9));

    auto g9 = r.getEquivalencyGroup(2);
    QCOMPARE(g9.size(), 3);
    QVERIFY(g9.contains(2));
    QVERIFY(g9.contains(5));
    QVERIFY(g9.contains(9));
}

void TestHashRelations::getOtherHashesEquivalentTo_resultDoesNotIncludeArgument()
{
    HashRelations r;
    r.markAsEquivalent({2, 5, 9});

    auto others = r.getOtherHashesEquivalentTo(5);
    QCOMPARE(others.size(), 2);
    QVERIFY(others.contains(2));
    QVERIFY(others.contains(9));

    QVERIFY(!others.contains(5));
}

void TestHashRelations::loadEquivalences()
{
    HashRelations r;
    r.loadEquivalences(
        {
            {1, 2},
            {3, 4},
            {5, 6},
            {22, 23},
            {2, 22},
            {1, 50}
        }
    );

    auto g2 = r.getEquivalencyGroup(2);
    QCOMPARE(g2.size(), 5);
    QVERIFY(g2.contains(1));
    QVERIFY(g2.contains(2));
    QVERIFY(g2.contains(22));
    QVERIFY(g2.contains(23));
    QVERIFY(g2.contains(50));

    auto g3 = r.getEquivalencyGroup(3);
    QCOMPARE(g3.size(), 2);
    QVERIFY(g3.contains(3));
    QVERIFY(g3.contains(4));

    auto g6 = r.getEquivalencyGroup(6);
    QCOMPARE(g6.size(), 2);
    QVERIFY(g6.contains(5));
    QVERIFY(g6.contains(6));
}

void TestHashRelations::markAsEquivalent()
{
    HashRelations r;
    r.markAsEquivalent({1, 2});
    r.markAsEquivalent({3, 4});
    r.markAsEquivalent({1, 8});

    auto g1 = r.getEquivalencyGroup(1);
    QCOMPARE(g1.size(), 3);
    QVERIFY(g1.contains(1));
    QVERIFY(g1.contains(2));
    QVERIFY(g1.contains(8));

    auto g3 = r.getEquivalencyGroup(3);
    QCOMPARE(g3.size(), 2);
    QVERIFY(g3.contains(3));
    QVERIFY(g3.contains(4));
}

void TestHashRelations::markAsEquivalent_joinsExistingGroups()
{
    HashRelations r;
    r.markAsEquivalent({1, 2, 3, 4, 5});
    r.markAsEquivalent({30, 40});
    r.markAsEquivalent({6, 7, 8, 9});
    r.markAsEquivalent({5, 6});

    auto g1 = r.getEquivalencyGroup(1);
    QCOMPARE(g1.size(), 9);

    for (int i = 1; i <= 9; ++i)
        QVERIFY(g1.contains(i));

    QVERIFY(!g1.contains(30));
    QVERIFY(!g1.contains(40));
}

QTEST_MAIN(TestHashRelations)
