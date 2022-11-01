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

#include "common/nullable.h"

#include <QtTest/QTest>

using namespace PMP;

class SpecialType
{
public:
    SpecialType(int* constructorCount, int* destructorCount)
     : _constructorCount(constructorCount),
       _destructorCount(destructorCount)
    {
        (*_constructorCount)++;
    }

    SpecialType(SpecialType const& other)
     : _constructorCount(other._constructorCount),
       _destructorCount(other._destructorCount)
    {
        (*_constructorCount)++;
    }

    ~SpecialType()
    {
        (*_destructorCount)++;
    }

    SpecialType& operator=(SpecialType const& other)
    {
        _constructorCount = other._constructorCount;
        _destructorCount = other._destructorCount;
        return *this;
    }

    void dummyOperation() {}

private:
    int* _constructorCount;
    int* _destructorCount;
};

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

void TestNullable::defaultConstructorWorksIfTypeNotDefaultConstructible()
{
    Nullable<SpecialType> n;

    QVERIFY(n.isNull());
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

void TestNullable::valueConstructedFromSpecialType()
{
    int constructed = 0;
    int destructed = 0;
    SpecialType t(&constructed, &destructed);

    QCOMPARE(constructed, 1);

    Nullable<SpecialType> n(t);

    QCOMPARE(constructed, 2);
}

void TestNullable::destructorCallsValueDestructor()
{
    int constructed = 0;
    int destructed = 0;
    SpecialType t(&constructed, &destructed);

    QCOMPARE(destructed, 0);

    {
        Nullable<SpecialType> n(t);
    }

    QCOMPARE(destructed, 1);
}

void TestNullable::setToNullCallsValueDestructor()
{
    int constructed = 0;
    int destructed = 0;
    SpecialType t(&constructed, &destructed);
    Nullable<SpecialType> n(t);

    QCOMPARE(destructed, 0);

    n.setToNull();

    QCOMPARE(destructed, 1);
}

void TestNullable::valueDoesNotCauseCopy()
{
    int constructed = 0;
    int destructed = 0;
    SpecialType t(&constructed, &destructed);
    Nullable<SpecialType> n(t);

    QCOMPARE(constructed, 2);

    auto& v = n.value();

    QCOMPARE(constructed, 2);

    v.dummyOperation(); /* avoids 'unused variable' warning */
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

void TestNullable::assignmentOperatorCallsValueConstructor()
{
    int constructed = 0;
    int destructed = 0;
    SpecialType t(&constructed, &destructed);
    Nullable<SpecialType> n1(t);
    Nullable<SpecialType> n2;

    // reset
    constructed = 0;
    destructed = 0;

    n2 = n1;

    QCOMPARE(constructed, 1);
}

void TestNullable::valueOrReturnsValueIfNotNull()
{
    Nullable<int> i { 1234 };
    Nullable<QString> s { QString("ABCD") };

    QCOMPARE(i.valueOr(-7), 1234);
    QCOMPARE(s.valueOr("Hello"), QString("ABCD"));
}

void TestNullable::valueOrReturnsAlternativeIfNull()
{
    Nullable<int> i;
    Nullable<QString> s;

    QCOMPARE(i.valueOr(-7), -7);
    QCOMPARE(s.valueOr("Hello"), QString("Hello"));
}

QTEST_MAIN(TestNullable)
