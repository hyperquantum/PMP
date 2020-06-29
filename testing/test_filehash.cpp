/*
    Copyright (C) 2020, Kevin Andre <hyperquantum@gmail.com>

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

#include "test_filehash.h"

#include "common/filehash.h"

#include <QtTest/QTest>

using namespace PMP;

void TestFileHash::knownHash1() {
    auto hash = FileHash::create("PMP");
    QCOMPARE(hash.length(), 3);
    QCOMPARE(hash.SHA1().toBase64(), "1oIJAa0hZTqtJL8nQavFDTBU+iM=");
    QCOMPARE(hash.MD5().toBase64(), "qpyo6LBWx0e+mL2NI6S32Q==");
    QCOMPARE(toString(hash), "3;1oIJAa0hZTqtJL8nQavFDTBU+iM;qpyo6LBWx0e+mL2NI6S32Q");
}

void TestFileHash::knownHash2() {
    auto hash = FileHash::create("6 stones");
    QCOMPARE(hash.length(), 8);
    QCOMPARE(toString(hash), "8;342OPoW+B/jnmCpic++f+L7pp8Q;ErGCCdD59y1QL4TKS8i8KQ");
}

void TestFileHash::knownHash3() {
    const char* text =
            "q5oh3rbazmu20c53yfwpzfvqukqsc7by14gztn816lqus04moml3xlpmvhfrhl0imka246"
            "b100e3fmlwzgxraua7h194ywtk7q83l3tj8f1m4tr5j9l1u5tw2p4b9d3e539sgf44kvri"
            "t9k0zxwkurz6w14ttji07ixwogqywh1ooh4ji7agil7fjfjs6oo4fgl31q4hd9ecgwuyyc"
            "a3o9zqysdtj8yss95w4ngtw524umxljufsmonob8htx9lf8uowjq6r5ic75ey3zpie754j";
    auto hash = FileHash::create(text);
    QCOMPARE(hash.length(), 280);
    QCOMPARE(toString(hash), "280;uvngJFDZ1g+tc96UllnxCqUHsqE;YxYHHbUCsw+Z+HICFMw9Rw");
}

void TestFileHash::knownHash4() {
    QByteArray bytes;
    bytes.append(char(22));
    bytes.append(char(1));
    bytes.append(char(127));
    bytes.append(char(32));

    auto hash = FileHash::create(bytes);
    QCOMPARE(hash.length(), 4);
    QCOMPARE(toString(hash), "4;EjEkmxYqMjEqfhvetDQlr+DXrUs;qrCXRS1P2DtxFCLT/Yfo0A");
}

QString TestFileHash::toString(const PMP::FileHash& hash) {
    return QString::number(hash.length())
            + ";" + hash.SHA1().toBase64(QByteArray::OmitTrailingEquals)
            + ";" + hash.MD5().toBase64(QByteArray::OmitTrailingEquals);
}

QTEST_MAIN(TestFileHash)
