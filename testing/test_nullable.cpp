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

#include "test_nullable.h"

#include <QtTest/QTest>

using namespace PMP;

void TestNullable::defaultConstructedIsNull()
{
    Nullable<int> i;
    Nullable<QString> s;

    QVERIFY(i.isNull());
    QVERIFY(s.isNull());

    QVERIFY(i == null);
    QVERIFY(s == null);

    QVERIFY(!i.hasValue());
    QVERIFY(!s.hasValue());
}

void TestNullable::nullConstructedIsNull()
{
    Nullable<int> i { null };
    Nullable<QString> s { null };

    QVERIFY(i.isNull());
    QVERIFY(s.isNull());

    QVERIFY(i == null);
    QVERIFY(s == null);

    QVERIFY(!i.hasValue());
    QVERIFY(!s.hasValue());
}

void TestNullable::valueConstructedIsNotNull()
{
    Nullable<int> i { 1234 };
    Nullable<QString> s { QString("ABCD") };

    QVERIFY(!i.isNull());
    QVERIFY(!s.isNull());

    QVERIFY(i != null);
    QVERIFY(s != null);

    QVERIFY(i.hasValue());
    QVERIFY(s.hasValue());
}

void TestNullable::valueConstructedContainsCorrectValue()
{
    Nullable<int> i { 1234 };
    Nullable<QString> s { QString("ABCD") };

    QCOMPARE(i.value(), 1234);
    QCOMPARE(s.value(), QString("ABCD"));
}

void TestNullable::equalsOperatorComparesValue()
{
    Nullable<int> i { 1234 };
    Nullable<QString> s { QString("ABCD") };

    QVERIFY(i == 1234);
    QVERIFY(s == QString("ABCD"));

    QVERIFY(!(i == 7777));
    QVERIFY(!(s == QString("COLD")));
}

void TestNullable::notEqualOperatorComparesValue()
{
    Nullable<int> i { 1234 };
    Nullable<QString> s { QString("ABCD") };

    QVERIFY(i != 7777);
    QVERIFY(s != QString("COLD"));

    QVERIFY(!(i != 1234));
    QVERIFY(!(s != QString("ABCD")));
}

void TestNullable::assignmentOperatorWorks()
{
    Nullable<int> i { 1234 };

    QVERIFY(i != null);
    QVERIFY(i == 1234);

    i = 5432;

    QVERIFY(i != null);
    QVERIFY(i == 5432);

    i = null;

    QVERIFY(i == null);
    QVERIFY(i != 1234);
    QVERIFY(i != 5432);

    i = 789;

    QVERIFY(i != null);
    QVERIFY(i == 789);
}

QTEST_MAIN(TestNullable)
