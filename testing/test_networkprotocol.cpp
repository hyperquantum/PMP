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

#include "test_networkprotocol.h"

#include "common/filehash.h"
#include "common/networkprotocol.h"

#include <QtTest/QTest>

using namespace PMP;

void TestNetworkProtocol::fileHashByteCount()
{
    /* this should never change */
    const int length = 8;
    const int sha1 = 20;
    const int md5 = 16;

    const int fileHashByteCount = NetworkProtocol::FILEHASH_BYTECOUNT; /* should be 44 */
    QCOMPARE(fileHashByteCount, length + sha1 + md5);
}

void TestNetworkProtocol::appendHash() {
    QByteArray content("Hello");
    FileHash hash = FileHash::create(content);

    QByteArray buffer;
    NetworkProtocol::appendHash(buffer, hash);

    QCOMPARE(buffer.size(), 44);
    QCOMPARE(buffer.mid(8, 20), hash.SHA1());
    QCOMPARE(buffer.mid(28, 16), hash.MD5());
}

void TestNetworkProtocol::getHash()
{
    QByteArray content("wonderful");
    FileHash hash = FileHash::create(content);

    QByteArray buffer;
    NetworkProtocol::appendHash(buffer, hash);

    bool ok;
    auto result = NetworkProtocol::getHash(buffer, 0, &ok);
    QCOMPARE(ok, true);
    QCOMPARE(result.length(), hash.length());
    QCOMPARE(result.SHA1(), hash.SHA1());
    QCOMPARE(result.MD5(), hash.MD5());
}

void TestNetworkProtocol::appendEmptyHash()
{
    FileHash emptyHash;

    QByteArray buffer;
    NetworkProtocol::appendHash(buffer, emptyHash);

    QCOMPARE(buffer.size(), 44);
    for (int i = 0 ; i < buffer.size(); ++i) {
        QCOMPARE(buffer[i], char(0));
    }
}

void TestNetworkProtocol::getEmptyHash()
{
    FileHash emptyHash;

    QByteArray buffer;
    NetworkProtocol::appendHash(buffer, emptyHash);

    bool ok;
    auto result = NetworkProtocol::getHash(buffer, 0, &ok);
    QCOMPARE(ok, true);
    QCOMPARE(result.length(), emptyHash.length());
    QCOMPARE(result.SHA1(), emptyHash.SHA1());
    QCOMPARE(result.MD5(), emptyHash.MD5());
}

QTEST_MAIN(TestNetworkProtocol)
