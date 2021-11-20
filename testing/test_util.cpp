/*
    Copyright (C) 2018-2021, Kevin Andre <hyperquantum@gmail.com>

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

void TestUtil::secondsToHoursMinuteSecondsText()
{
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

    QCOMPARE(Util::millisecondsToShortDisplayTimeText(-30000), QString("-00:30.0"));
    QCOMPARE(Util::millisecondsToShortDisplayTimeText(-30001), QString("-00:30.0"));
    QCOMPARE(Util::millisecondsToShortDisplayTimeText(-30099), QString("-00:30.0"));
    QCOMPARE(Util::millisecondsToShortDisplayTimeText(-30100), QString("-00:30.1"));
    QCOMPARE(Util::millisecondsToShortDisplayTimeText(-45000), QString("-00:45.0"));
    QCOMPARE(Util::millisecondsToShortDisplayTimeText(-59999), QString("-00:59.9"));
    QCOMPARE(Util::millisecondsToShortDisplayTimeText(-60000), QString("-01:00.0"));
    QCOMPARE(Util::millisecondsToShortDisplayTimeText(-60099), QString("-01:00.0"));
    QCOMPARE(Util::millisecondsToShortDisplayTimeText(-60100), QString("-01:00.1"));
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

    QCOMPARE(Util::millisecondsToLongDisplayTimeText(-30000), QString("-00:00:30.000"));
    QCOMPARE(Util::millisecondsToLongDisplayTimeText(-30001), QString("-00:00:30.001"));
    QCOMPARE(Util::millisecondsToLongDisplayTimeText(-30099), QString("-00:00:30.099"));
    QCOMPARE(Util::millisecondsToLongDisplayTimeText(-30100), QString("-00:00:30.100"));
    QCOMPARE(Util::millisecondsToLongDisplayTimeText(-45000), QString("-00:00:45.000"));
    QCOMPARE(Util::millisecondsToLongDisplayTimeText(-59999), QString("-00:00:59.999"));
    QCOMPARE(Util::millisecondsToLongDisplayTimeText(-60000), QString("-00:01:00.000"));
    QCOMPARE(Util::millisecondsToLongDisplayTimeText(-60099), QString("-00:01:00.099"));
    QCOMPARE(Util::millisecondsToLongDisplayTimeText(-60100), QString("-00:01:00.100"));
}

void TestUtil::getHowLongAgoText()
{
    const int seconds = 1;
    const int minutes = 60 * seconds;
    const int hours = 60 * minutes;
    const int days = 24 * hours;
    const int weeks = 7 * days;

    auto howLongAgoText =
        [](int seconds)
        {
            return Util::getHowLongAgoText(Util::getHowLongAgoDuration(seconds));
        };

    QCOMPARE(howLongAgoText(0), QString("just now"));

    QCOMPARE(howLongAgoText(1 * seconds), QString("1 second ago"));
    QCOMPARE(howLongAgoText(1 * minutes), QString("1 minute ago"));
    QCOMPARE(howLongAgoText(1 * hours), QString("1 hour ago"));
    QCOMPARE(howLongAgoText(1 * days), QString("1 day ago"));
    QCOMPARE(howLongAgoText(1 * weeks), QString("1 week ago"));
    QCOMPARE(howLongAgoText(30 * days), QString("1 month ago"));
    QCOMPARE(howLongAgoText(365 * days), QString("1 year ago"));

    QCOMPARE(howLongAgoText(2 * seconds), QString("2 seconds ago"));
    QCOMPARE(howLongAgoText(2 * minutes), QString("2 minutes ago"));
    QCOMPARE(howLongAgoText(2 * hours), QString("2 hours ago"));
    QCOMPARE(howLongAgoText(2 * days), QString("2 days ago"));
    QCOMPARE(howLongAgoText(2 * weeks), QString("2 weeks ago"));
    QCOMPARE(howLongAgoText(2 * 30 * days), QString("2 months ago"));
    QCOMPARE(howLongAgoText(2 * 365 * days), QString("2 years ago"));

    QCOMPARE(howLongAgoText(59 * seconds), QString("59 seconds ago"));
    QCOMPARE(howLongAgoText(59 * minutes), QString("59 minutes ago"));
    QCOMPARE(howLongAgoText(23 * hours), QString("23 hours ago"));
    QCOMPARE(howLongAgoText(6 * days), QString("6 days ago"));
    QCOMPARE(howLongAgoText(29 * days), QString("4 weeks ago"));
    QCOMPARE(howLongAgoText(364 * days), QString("12 months ago"));

    QCOMPARE(howLongAgoText(1 * minutes - 1 * seconds), QString("59 seconds ago"));
    QCOMPARE(howLongAgoText(1 * hours - 1 * seconds), QString("59 minutes ago"));
    QCOMPARE(howLongAgoText(1 * days - 1 * seconds), QString("23 hours ago"));
    QCOMPARE(howLongAgoText(1 * weeks - 1 * seconds), QString("6 days ago"));
    QCOMPARE(howLongAgoText(30 * days - 1 * seconds), QString("4 weeks ago"));
    QCOMPARE(howLongAgoText(365 * days - 1 * seconds), QString("12 months ago"));

    QCOMPARE(howLongAgoText(3 * 30 * days), QString("2 months ago"));
    QCOMPARE(howLongAgoText((3 * 30 + 1) * days), QString("3 months ago"));
    QCOMPARE(howLongAgoText(4 * 30 * days), QString("3 months ago"));
    QCOMPARE(howLongAgoText((4 * 30 + 1) * days), QString("4 months ago"));
    QCOMPARE(howLongAgoText(5 * 30 * days), QString("4 months ago"));
    QCOMPARE(howLongAgoText((5 * 30 + 1) * days), QString("5 months ago"));
    QCOMPARE(howLongAgoText(6 * 30 * days), QString("5 months ago"));
    QCOMPARE(howLongAgoText((6 * 30 + 1) * days), QString("5 months ago"));
    QCOMPARE(howLongAgoText((6 * 30 + 2) * days), QString("6 months ago"));
    QCOMPARE(howLongAgoText(7 * 30 * days), QString("6 months ago"));
    QCOMPARE(howLongAgoText((7 * 30 + 1) * days), QString("6 months ago"));
    QCOMPARE(howLongAgoText((7 * 30 + 2) * days), QString("7 months ago"));
    QCOMPARE(howLongAgoText(8 * 30 * days), QString("7 months ago"));
    QCOMPARE(howLongAgoText((8 * 30 + 1) * days), QString("7 months ago"));
    QCOMPARE(howLongAgoText((8 * 30 + 2) * days), QString("8 months ago"));
    QCOMPARE(howLongAgoText(9 * 30 * days), QString("8 months ago"));
    QCOMPARE(howLongAgoText((9 * 30 + 1) * days), QString("8 months ago"));
    QCOMPARE(howLongAgoText((9 * 30 + 2) * days), QString("8 months ago"));
    QCOMPARE(howLongAgoText((9 * 30 + 3) * days), QString("9 months ago"));
    QCOMPARE(howLongAgoText(10 * 30 * days), QString("9 months ago"));
    QCOMPARE(howLongAgoText((10 * 30 + 1) * days), QString("9 months ago"));
    QCOMPARE(howLongAgoText((10 * 30 + 2) * days), QString("9 months ago"));
    QCOMPARE(howLongAgoText((10 * 30 + 3) * days), QString("10 months ago"));
    QCOMPARE(howLongAgoText(11 * 30 * days), QString("10 months ago"));
    QCOMPARE(howLongAgoText((11 * 30 + 1) * days), QString("10 months ago"));
    QCOMPARE(howLongAgoText((11 * 30 + 2) * days), QString("10 months ago"));
    QCOMPARE(howLongAgoText((11 * 30 + 3) * days), QString("11 months ago"));
    QCOMPARE(howLongAgoText(12 * 30 * days), QString("11 months ago"));
    QCOMPARE(howLongAgoText((12 * 30 + 1) * days), QString("11 months ago"));
    QCOMPARE(howLongAgoText((12 * 30 + 2) * days), QString("11 months ago"));
    QCOMPARE(howLongAgoText((12 * 30 + 3) * days), QString("11 months ago"));
    QCOMPARE(howLongAgoText((12 * 30 + 4) * days), QString("12 months ago"));

    QCOMPARE(howLongAgoText(3 * 365 * days), QString("3 years ago"));
    QCOMPARE(howLongAgoText(4 * 365 * days), QString("3 years ago"));
    QCOMPARE(howLongAgoText((3 * 365 + 366) * days), QString("4 years ago"));
    QCOMPARE(howLongAgoText(5 * 365 * days), QString("4 years ago"));
    QCOMPARE(howLongAgoText((4 * 365 + 366) * days), QString("5 years ago"));
    QCOMPARE(howLongAgoText(6 * 365 * days), QString("5 years ago"));
    QCOMPARE(howLongAgoText((5 * 365 + 366) * days), QString("6 years ago"));
    QCOMPARE(howLongAgoText(7 * 365 * days), QString("6 years ago"));
    QCOMPARE(howLongAgoText((6 * 365 + 366) * days), QString("7 years ago"));
    QCOMPARE(howLongAgoText(8 * 365 * days), QString("7 years ago"));
    QCOMPARE(howLongAgoText((7 * 365 + 366) * days), QString("7 years ago"));
    QCOMPARE(howLongAgoText((6 * 365 + 2 * 366) * days), QString("8 years ago"));
}

void TestUtil::getHowLongAgoUpdateIntervalMs()
{
    QCOMPARE(Util::getHowLongAgoUpdateIntervalMs(0), 250);
    QCOMPARE(Util::getHowLongAgoUpdateIntervalMs(1), 250);
    QCOMPARE(Util::getHowLongAgoUpdateIntervalMs(9), 250);
    QCOMPARE(Util::getHowLongAgoUpdateIntervalMs(59), 250);
    QCOMPARE(Util::getHowLongAgoUpdateIntervalMs(60), 1000);
    QCOMPARE(Util::getHowLongAgoUpdateIntervalMs(100), 1000);
    QCOMPARE(Util::getHowLongAgoUpdateIntervalMs(3599), 1000);
    QCOMPARE(Util::getHowLongAgoUpdateIntervalMs(3600), 60 * 1000);
    QCOMPARE(Util::getHowLongAgoUpdateIntervalMs(24 * 3600), 60 * 1000);
    QCOMPARE(Util::getHowLongAgoUpdateIntervalMs(7 * 24 * 3600), 60 * 1000);
    QCOMPARE(Util::getHowLongAgoUpdateIntervalMs(365 * 24 * 3600), 60 * 1000);
}

void TestUtil::getCopyrightLine()
{
    QVERIFY(Util::getCopyrightLine(false).contains(QString("Copyright")));
    QVERIFY(Util::getCopyrightLine(true).contains(QString("Copyright")));

    QVERIFY(Util::getCopyrightLine(false).contains(Util::Copyright));
    QVERIFY(Util::getCopyrightLine(true).contains(QString("(C)")));

    QVERIFY(Util::getCopyrightLine(false).contains(QString("Kevin Andr") + Util::EAcute));
    QVERIFY(Util::getCopyrightLine(true).contains(QString("Kevin Andre")));

    auto copyrightAscii = Util::getCopyrightLine(true);
    for (int i = 0; i < copyrightAscii.size(); ++i)
    {
        QVERIFY(copyrightAscii[i] < QChar(128));
    }
}

void TestUtil::getRandomSeed()
{
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

void TestUtil::generateZeroedMemory()
{
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
