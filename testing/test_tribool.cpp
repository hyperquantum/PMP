/*
    Copyright (C) 2024, Kevin Andre <hyperquantum@gmail.com>

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

#include "test_tribool.h"

#include "common/tribool.h"

#include <QtTest/QTest>

using namespace PMP;

void TestTriBool::defaultConstructedIsUnknown()
{
    QCOMPARE(TriBool().isUnknown(), true);
    QCOMPARE(TriBool().isKnown(), false);
}

void TestTriBool::constructedFromBoolIsKnown()
{
    QCOMPARE(TriBool(true).isUnknown(), false);
    QCOMPARE(TriBool(true).isKnown(), true);

    QCOMPARE(TriBool(true).isUnknown(), false);
    QCOMPARE(TriBool(true).isKnown(), true);
}

void TestTriBool::staticUnknownIsUnknown()
{
    QCOMPARE(TriBool::unknown.isUnknown(), true);
    QCOMPARE(TriBool::unknown.isKnown(), false);
}

void TestTriBool::isTrueWorksCorrectly()
{
    QCOMPARE(TriBool(true).isTrue(), true);
    QCOMPARE(TriBool(false).isTrue(), false);
    QCOMPARE(TriBool::unknown.isTrue(), false);
}

void TestTriBool::isFalseWorksCorrectly()
{
    QCOMPARE(TriBool(true).isFalse(), false);
    QCOMPARE(TriBool(false).isFalse(), true);
    QCOMPARE(TriBool::unknown.isFalse(), false);
}

void TestTriBool::toBoolWorksCorrectly()
{
    QCOMPARE(TriBool(true).toBool(), true);
    QCOMPARE(TriBool(false).toBool(), false);
    QCOMPARE(TriBool::unknown.toBool(), false);

    QCOMPARE(TriBool(true).toBool(false), true);
    QCOMPARE(TriBool(false).toBool(false), false);
    QCOMPARE(TriBool::unknown.toBool(false), false);

    QCOMPARE(TriBool(true).toBool(true), true);
    QCOMPARE(TriBool(false).toBool(true), false);
    QCOMPARE(TriBool::unknown.toBool(true), true);
}

void TestTriBool::isIdenticalToWorksCorrectly()
{
    QCOMPARE(TriBool(true).isIdenticalTo(TriBool(true)), true);
    QCOMPARE(TriBool(false).isIdenticalTo(TriBool(true)), false);
    QCOMPARE(TriBool::unknown.isIdenticalTo(TriBool(true)), false);

    QCOMPARE(TriBool(true).isIdenticalTo(TriBool(false)), false);
    QCOMPARE(TriBool(false).isIdenticalTo(TriBool(false)), true);
    QCOMPARE(TriBool::unknown.isIdenticalTo(TriBool(false)), false);

    QCOMPARE(TriBool(true).isIdenticalTo(TriBool::unknown), false);
    QCOMPARE(TriBool(false).isIdenticalTo(TriBool::unknown), false);
    QCOMPARE(TriBool::unknown.isIdenticalTo(TriBool::unknown), true);
}

void TestTriBool::notOperatorWorksCorrectly()
{
    QVERIFY((!TriBool(true)).isFalse());
    QVERIFY((!TriBool(false)).isTrue());
    QVERIFY((!TriBool::unknown).isUnknown());
}

void TestTriBool::equalsOperatorWorksCorrectly()
{
    QVERIFY((TriBool(true) == TriBool(true)).isTrue());
    QVERIFY((TriBool(false) == TriBool(true)).isFalse());
    QVERIFY((TriBool::unknown == TriBool(true)).isUnknown());

    QVERIFY((TriBool(true) == TriBool(false)).isFalse());
    QVERIFY((TriBool(false) == TriBool(false)).isTrue());
    QVERIFY((TriBool::unknown == TriBool(false)).isUnknown());

    QVERIFY((TriBool(true) == TriBool::unknown).isUnknown());
    QVERIFY((TriBool(false) == TriBool::unknown).isUnknown());
    QVERIFY((TriBool::unknown == TriBool::unknown).isUnknown());
}

void TestTriBool::differsOperatorWorksCorrectly()
{
    QVERIFY((TriBool(true) != TriBool(true)).isFalse());
    QVERIFY((TriBool(false) != TriBool(true)).isTrue());
    QVERIFY((TriBool::unknown != TriBool(true)).isUnknown());

    QVERIFY((TriBool(true) != TriBool(false)).isTrue());
    QVERIFY((TriBool(false) != TriBool(false)).isFalse());
    QVERIFY((TriBool::unknown != TriBool(false)).isUnknown());

    QVERIFY((TriBool(true) != TriBool::unknown).isUnknown());
    QVERIFY((TriBool(false) != TriBool::unknown).isUnknown());
    QVERIFY((TriBool::unknown != TriBool::unknown).isUnknown());
}

void TestTriBool::andOperatorWorksCorrectly()
{
    QVERIFY((TriBool(true) & TriBool(true)).isTrue());
    QVERIFY((TriBool(false) & TriBool(true)).isFalse());
    QVERIFY((TriBool::unknown & TriBool(true)).isUnknown());

    QVERIFY((TriBool(true) & TriBool(false)).isFalse());
    QVERIFY((TriBool(false) & TriBool(false)).isFalse());
    QVERIFY((TriBool::unknown & TriBool(false)).isFalse());

    QVERIFY((TriBool(true) & TriBool::unknown).isUnknown());
    QVERIFY((TriBool(false) & TriBool::unknown).isFalse());
    QVERIFY((TriBool::unknown & TriBool::unknown).isUnknown());
}

void TestTriBool::orOperatorWorksCorrectly()
{
    QVERIFY((TriBool(true) | TriBool(true)).isTrue());
    QVERIFY((TriBool(false) | TriBool(true)).isTrue());
    QVERIFY((TriBool::unknown | TriBool(true)).isTrue());

    QVERIFY((TriBool(true) | TriBool(false)).isTrue());
    QVERIFY((TriBool(false) | TriBool(false)).isFalse());
    QVERIFY((TriBool::unknown | TriBool(false)).isUnknown());

    QVERIFY((TriBool(true) | TriBool::unknown).isTrue());
    QVERIFY((TriBool(false) | TriBool::unknown).isUnknown());
    QVERIFY((TriBool::unknown | TriBool::unknown).isUnknown());
}

QTEST_MAIN(TestTriBool)
