/*
    Copyright (C) 2018-2020, Kevin Andre <hyperquantum@gmail.com>

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

#include "test_util.h"

#include "common/util.h"

#include <QtTest/QTest>

using namespace PMP;

void TestUtil::secondsToHoursMinuteSecondsText() {
    QCOMPARE(Util::secondsToHoursMinuteSecondsText(30), QString("00:00:30"));
    QCOMPARE(Util::secondsToHoursMinuteSecondsText(60), QString("00:01:00"));
}

void TestUtil::millisecondsToShortDisplayTimeText()
{
    QCOMPARE(Util::millisecondsToShortDisplayTimeText(30000), QString("00:30.0"));
    QCOMPARE(Util::millisecondsToShortDisplayTimeText(30001), QString("00:30.0"));
    QCOMPARE(Util::millisecondsToShortDisplayTimeText(30099), QString("00:30.0"));
    QCOMPARE(Util::millisecondsToShortDisplayTimeText(30100), QString("00:30.1"));
    QCOMPARE(Util::millisecondsToShortDisplayTimeText(45000), QString("00:45.0"));
    QCOMPARE(Util::millisecondsToShortDisplayTimeText(59999), QString("00:59.9"));
    QCOMPARE(Util::millisecondsToShortDisplayTimeText(60000), QString("01:00.0"));
    QCOMPARE(Util::millisecondsToShortDisplayTimeText(60099), QString("01:00.0"));
    QCOMPARE(Util::millisecondsToShortDisplayTimeText(60100), QString("01:00.1"));

    QCOMPARE(Util::millisecondsToShortDisplayTimeText(3599999), QString("59:59.9"));
    QCOMPARE(Util::millisecondsToShortDisplayTimeText(3600000), QString("01:00:00.0"));
    QCOMPARE(Util::millisecondsToShortDisplayTimeText(3644500), QString("01:00:44.5"));
}

void TestUtil::millisecondsToLongDisplayTimeText()
{
    QCOMPARE(Util::millisecondsToLongDisplayTimeText(30000), QString("00:00:30.000"));
    QCOMPARE(Util::millisecondsToLongDisplayTimeText(30001), QString("00:00:30.001"));
    QCOMPARE(Util::millisecondsToLongDisplayTimeText(30099), QString("00:00:30.099"));
    QCOMPARE(Util::millisecondsToLongDisplayTimeText(30100), QString("00:00:30.100"));
    QCOMPARE(Util::millisecondsToLongDisplayTimeText(45000), QString("00:00:45.000"));
    QCOMPARE(Util::millisecondsToLongDisplayTimeText(59999), QString("00:00:59.999"));
    QCOMPARE(Util::millisecondsToLongDisplayTimeText(60000), QString("00:01:00.000"));
    QCOMPARE(Util::millisecondsToLongDisplayTimeText(60099), QString("00:01:00.099"));
    QCOMPARE(Util::millisecondsToLongDisplayTimeText(60100), QString("00:01:00.100"));

    QCOMPARE(Util::millisecondsToLongDisplayTimeText(3599999), QString("00:59:59.999"));
    QCOMPARE(Util::millisecondsToLongDisplayTimeText(3600000), QString("01:00:00.000"));
    QCOMPARE(Util::millisecondsToLongDisplayTimeText(3644500), QString("01:00:44.500"));
}

void TestUtil::getCopyrightLine() {
    QVERIFY(Util::getCopyrightLine(false).contains(QString("Copyright")));
    QVERIFY(Util::getCopyrightLine(true).contains(QString("Copyright")));

    QVERIFY(Util::getCopyrightLine(false).contains(Util::Copyright));
    QVERIFY(Util::getCopyrightLine(true).contains(QString("(C)")));

    QVERIFY(Util::getCopyrightLine(false).contains(QString("Kevin Andr") + Util::EAcute));
    QVERIFY(Util::getCopyrightLine(true).contains(QString("Kevin Andre")));

    auto copyrightAscii = Util::getCopyrightLine(true);
    for (int i = 0; i < copyrightAscii.size(); ++i) {
        QVERIFY(copyrightAscii[i] < QChar(128));
    }
}

void TestUtil::getRandomSeed() {
    auto seed1 = Util::getRandomSeed();
    auto seed2 = Util::getRandomSeed();
    auto seed3 = Util::getRandomSeed();
    auto seed4 = Util::getRandomSeed();
    QVERIFY(seed1 != seed2);
    QVERIFY(seed1 != seed3);
    QVERIFY(seed1 != seed4);
    QVERIFY(seed2 != seed3);
    QVERIFY(seed2 != seed4);
    QVERIFY(seed3 != seed4);
}

void TestUtil::generateZeroedMemory() {
    auto a = Util::generateZeroedMemory(29);
    auto b = Util::generateZeroedMemory(64);
    auto c = Util::generateZeroedMemory(29);

    QCOMPARE(a.size(), 29);
    QCOMPARE(b.size(), 64);
    QCOMPARE(c.size(), 29);

    QVERIFY(a != b);
    QVERIFY(a == c);
    QVERIFY(b != c);

    for(int i = 0; i < a.size(); ++i)
        QVERIFY(a[i] == char(0));

    for(int i = 0; i < b.size(); ++i)
        QVERIFY(b[i] == char(0));

    for(int i = 0; i < c.size(); ++i)
        QVERIFY(c[i] == char(0));
}

QTEST_MAIN(TestUtil)
