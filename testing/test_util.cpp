/*
    Copyright (C) 2018, Kevin Andre <hyperquantum@gmail.com>

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

QTEST_MAIN(TestUtil)
