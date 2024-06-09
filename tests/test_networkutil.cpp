/*
    Copyright (C) 2019-2020, Kevin Andre <hyperquantum@gmail.com>

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

#include "test_networkutil.h"

#include "common/networkutil.h"

#include <QtTest/QTest>

using namespace PMP;

void TestNetworkUtil::fitsIn2BytesSigned()
{
    QVERIFY(NetworkUtil::fitsIn2BytesSigned(0));
    QVERIFY(NetworkUtil::fitsIn2BytesSigned(32767));
    QVERIFY(NetworkUtil::fitsIn2BytesSigned(-32768));

    QVERIFY(!NetworkUtil::fitsIn2BytesSigned(32768));
    QVERIFY(!NetworkUtil::fitsIn2BytesSigned(-32769));
}

void TestNetworkUtil::to2BytesSigned()
{
    {
        bool error = false;
        qint16 result = NetworkUtil::to2BytesSigned(32767, error, "test1");
        QCOMPARE(result, 32767);
        QCOMPARE(error, false);
    }
    {
        bool error = false;
        qint16 result = NetworkUtil::to2BytesSigned(-32768, error, "test1");
        QCOMPARE(result, -32768);
        QCOMPARE(error, false);
    }
    {
        bool error = false;
        qint16 result = NetworkUtil::to2BytesSigned(32768, error, "test1");
        QCOMPARE(result, 0);
        QCOMPARE(error, true);
    }
    {
        bool error = false;
        qint16 result = NetworkUtil::to2BytesSigned(-32769, error, "test1");
        QCOMPARE(result, 0);
        QCOMPARE(error, true);
    }
}

void TestNetworkUtil::appendByte() {
    QByteArray array;
    NetworkUtil::appendByte(array, '\0');
    NetworkUtil::appendByte(array, uchar(9));
    NetworkUtil::appendByte(array, uchar(30));
    NetworkUtil::appendByte(array, uchar(73));
    NetworkUtil::appendByte(array, uchar(127));
    NetworkUtil::appendByte(array, uchar(255));

    QCOMPARE(array.size(), 6);

    QCOMPARE(char(array[0]), '\0');
    QCOMPARE(char(array[1]), char(9));
    QCOMPARE(char(array[2]), char(30));
    QCOMPARE(char(array[3]), char(73));
    QCOMPARE(char(array[4]), char(127));
    QCOMPARE(char(array[5]), char(255));
}

void TestNetworkUtil::getByte() {
    QByteArray array;
    array.append('\0');
    array.append(char(9));
    array.append(char(30));
    array.append(char(73));
    array.append(char(127));
    array.append(char(255));

    QCOMPARE(array.size(), 6);

    QCOMPARE(NetworkUtil::getByte(array, 0), uchar('\0'));
    QCOMPARE(NetworkUtil::getByte(array, 1), uchar(9));
    QCOMPARE(NetworkUtil::getByte(array, 2), uchar(30));
    QCOMPARE(NetworkUtil::getByte(array, 3), uchar(73));
    QCOMPARE(NetworkUtil::getByte(array, 4), uchar(127));
    QCOMPARE(NetworkUtil::getByte(array, 5), uchar(255));
}

void TestNetworkUtil::append2Bytes() {
    QByteArray array;
    NetworkUtil::append2Bytes(array, 0u);
    NetworkUtil::append2Bytes(array, 30u);
    NetworkUtil::append2Bytes(array, 127u);
    NetworkUtil::append2Bytes(array, 255u);
    NetworkUtil::append2Bytes(array, 256u);
    NetworkUtil::append2Bytes(array, 8765u);
    NetworkUtil::append2Bytes(array, 26587u);
    NetworkUtil::append2Bytes(array, 65535u);

    QCOMPARE(array.size(), 8 * 2);

    QCOMPARE(char(array[0]), '\0');
    QCOMPARE(char(array[1]), '\0');

    QCOMPARE(char(array[2]), '\0');
    QCOMPARE(char(array[3]), char(30));

    QCOMPARE(char(array[4]), '\0');
    QCOMPARE(char(array[5]), char(127));

    QCOMPARE(char(array[6]), '\0');
    QCOMPARE(char(array[7]), char(255));

    QCOMPARE(char(array[8]), char(1));
    QCOMPARE(char(array[9]), char(0));

    QCOMPARE(char(array[10]), '\x22');
    QCOMPARE(char(array[11]), '\x3D');

    QCOMPARE(char(array[12]), '\x67');
    QCOMPARE(char(array[13]), '\xDB');

    QCOMPARE(char(array[14]), '\xFF');
    QCOMPARE(char(array[15]), '\xFF');
}

void TestNetworkUtil::get2Bytes() {
    QByteArray array;
    array.append('\0');
    array.append('\0');

    array.append('\0');
    array.append(char(30));

    array.append('\0');
    array.append(char(127));

    array.append('\0');
    array.append(char(255));

    array.append(char(1));
    array.append(char(0));

    array.append('\x22');
    array.append('\x3D');

    array.append('\x67');
    array.append('\xDB');

    array.append('\xFF');
    array.append('\xFF');

    QCOMPARE(array.size(), 8 * 2);

    QCOMPARE(NetworkUtil::get2Bytes(array, 0), quint16(0u));
    QCOMPARE(NetworkUtil::get2Bytes(array, 2), quint16(30u));
    QCOMPARE(NetworkUtil::get2Bytes(array, 4), quint16(127u));
    QCOMPARE(NetworkUtil::get2Bytes(array, 6), quint16(255u));
    QCOMPARE(NetworkUtil::get2Bytes(array, 8), quint16(256u));
    QCOMPARE(NetworkUtil::get2Bytes(array, 10), quint16(8765u));
    QCOMPARE(NetworkUtil::get2Bytes(array, 12), quint16(26587u));
    QCOMPARE(NetworkUtil::get2Bytes(array, 14), quint16(65535u));
}

void TestNetworkUtil::append4Bytes() {
    QByteArray array;
    NetworkUtil::append4Bytes(array, 0u);
    NetworkUtil::append4Bytes(array, 5544u);
    NetworkUtil::append4Bytes(array, 34088u);
    NetworkUtil::append4Bytes(array, 9605332u);
    NetworkUtil::append4Bytes(array, 4222618390u);

    QCOMPARE(array.size(), 5 * 4);

    QCOMPARE(char(array[0]), '\0');
    QCOMPARE(char(array[1]), '\0');
    QCOMPARE(char(array[2]), '\0');
    QCOMPARE(char(array[3]), '\0');

    QCOMPARE(char(array[4]), '\0');
    QCOMPARE(char(array[5]), '\0');
    QCOMPARE(char(array[6]), '\x15');
    QCOMPARE(char(array[7]), '\xA8');

    QCOMPARE(char(array[8]), '\0');
    QCOMPARE(char(array[9]), '\0');
    QCOMPARE(char(array[10]), '\x85');
    QCOMPARE(char(array[11]), '\x28');

    QCOMPARE(char(array[12]), '\0');
    QCOMPARE(char(array[13]), '\x92');
    QCOMPARE(char(array[14]), '\x90');
    QCOMPARE(char(array[15]), '\xD4');

    QCOMPARE(char(array[16]), '\xFB');
    QCOMPARE(char(array[17]), '\xB0');
    QCOMPARE(char(array[18]), '\x0B');
    QCOMPARE(char(array[19]), '\x16');
}

void TestNetworkUtil::get4Bytes() {
    QByteArray array;
    array.append('\0');
    array.append('\0');
    array.append('\0');
    array.append('\0');

    array.append('\0');
    array.append('\0');
    array.append('\x15');
    array.append('\xA8');

    array.append('\0');
    array.append('\0');
    array.append('\x85');
    array.append('\x28');

    array.append('\0');
    array.append('\x92');
    array.append('\x90');
    array.append('\xD4');

    array.append('\xFB');
    array.append('\xB0');
    array.append('\x0B');
    array.append('\x16');

    QCOMPARE(array.size(), 5 * 4);

    QCOMPARE(NetworkUtil::get4Bytes(array, 0), quint32(0u));
    QCOMPARE(NetworkUtil::get4Bytes(array, 4), quint32(5544u));
    QCOMPARE(NetworkUtil::get4Bytes(array, 8), quint32(34088u));
    QCOMPARE(NetworkUtil::get4Bytes(array, 12), quint32(9605332u));
    QCOMPARE(NetworkUtil::get4Bytes(array, 16), quint32(4222618390u));
}

void TestNetworkUtil::append8Bytes() {
    QByteArray array;
    NetworkUtil::append8Bytes(array, 0u);
    NetworkUtil::append8Bytes(array, 56542215u);
    NetworkUtil::append8Bytes(array, 9067630524680188ull);
    NetworkUtil::append8Bytes(array, 0x8000000000000000ull);
    NetworkUtil::append8Bytes(array, 0xFE2A54BB12CF5415ull);

    QCOMPARE(array.size(), 5 * 8);

    QCOMPARE(char(array[0]), '\0');
    QCOMPARE(char(array[1]), '\0');
    QCOMPARE(char(array[2]), '\0');
    QCOMPARE(char(array[3]), '\0');
    QCOMPARE(char(array[4]), '\0');
    QCOMPARE(char(array[5]), '\0');
    QCOMPARE(char(array[6]), '\0');
    QCOMPARE(char(array[7]), '\0');

    QCOMPARE(char(array[8]), '\0');
    QCOMPARE(char(array[9]), '\0');
    QCOMPARE(char(array[10]), '\0');
    QCOMPARE(char(array[11]), '\0');
    QCOMPARE(char(array[12]), '\x03');
    QCOMPARE(char(array[13]), '\x5E');
    QCOMPARE(char(array[14]), '\xC4');
    QCOMPARE(char(array[15]), '\x07');

    QCOMPARE(char(array[16]), '\0');
    QCOMPARE(char(array[17]), '\x20');
    QCOMPARE(char(array[18]), '\x36');
    QCOMPARE(char(array[19]), '\xF6');
    QCOMPARE(char(array[20]), '\x40');
    QCOMPARE(char(array[21]), '\x60');
    QCOMPARE(char(array[22]), '\xC7');
    QCOMPARE(char(array[23]), '\xFC');

    QCOMPARE(char(array[24]), '\x80');
    QCOMPARE(char(array[25]), '\x00');
    QCOMPARE(char(array[26]), '\x00');
    QCOMPARE(char(array[27]), '\x00');
    QCOMPARE(char(array[28]), '\x00');
    QCOMPARE(char(array[29]), '\x00');
    QCOMPARE(char(array[30]), '\x00');
    QCOMPARE(char(array[31]), '\x00');

    QCOMPARE(char(array[32]), '\xFE');
    QCOMPARE(char(array[33]), '\x2A');
    QCOMPARE(char(array[34]), '\x54');
    QCOMPARE(char(array[35]), '\xBB');
    QCOMPARE(char(array[36]), '\x12');
    QCOMPARE(char(array[37]), '\xCF');
    QCOMPARE(char(array[38]), '\x54');
    QCOMPARE(char(array[39]), '\x15');
}

void TestNetworkUtil::get8Bytes() {
    QByteArray array;
    array.append('\0');
    array.append('\0');
    array.append('\0');
    array.append('\0');
    array.append('\0');
    array.append('\0');
    array.append('\0');
    array.append('\0');

    array.append('\0');
    array.append('\0');
    array.append('\0');
    array.append('\0');
    array.append('\x03');
    array.append('\x5E');
    array.append('\xC4');
    array.append('\x07');

    array.append('\0');
    array.append('\x20');
    array.append('\x36');
    array.append('\xF6');
    array.append('\x40');
    array.append('\x60');
    array.append('\xC7');
    array.append('\xFC');

    array.append('\x80');
    array.append('\x00');
    array.append('\x00');
    array.append('\x00');
    array.append('\x00');
    array.append('\x00');
    array.append('\x00');
    array.append('\x00');

    array.append('\xFE');
    array.append('\x2A');
    array.append('\x54');
    array.append('\xBB');
    array.append('\x12');
    array.append('\xCF');
    array.append('\x54');
    array.append('\x15');

    QCOMPARE(array.size(), 5 * 8);

    QCOMPARE(NetworkUtil::get8Bytes(array, 0), quint64(0u));
    QCOMPARE(NetworkUtil::get8Bytes(array, 8), quint64(56542215u));
    QCOMPARE(NetworkUtil::get8Bytes(array, 16), quint64(9067630524680188ull));
    QCOMPARE(NetworkUtil::get8Bytes(array, 24), quint64(0x8000000000000000ull));
    QCOMPARE(NetworkUtil::get8Bytes(array, 32), quint64(0xFE2A54BB12CF5415ull));
}

void TestNetworkUtil::get2BytesSigned() {
    QByteArray array;
    array.append('\xFF');
    array.append('\xFF');

    array.append('\xFF');
    array.append('\xFE');

    array.append('\0');
    array.append('\x05');

    QCOMPARE(array.size(), 3 * 2);

    QCOMPARE(NetworkUtil::get2BytesSigned(array, 0), qint16(-1));
    QCOMPARE(NetworkUtil::get2BytesSigned(array, 2), qint16(-2));
    QCOMPARE(NetworkUtil::get2BytesSigned(array, 4), qint16(5));
}

void TestNetworkUtil::get4BytesSigned() {
    QByteArray array;
    array.append('\xFF');
    array.append('\xFF');
    array.append('\xFF');
    array.append('\xFF');

    array.append('\xFF');
    array.append('\xFF');
    array.append('\xFF');
    array.append('\xFE');

    array.append('\0');
    array.append('\0');
    array.append('\0');
    array.append('\x05');

    QCOMPARE(array.size(), 3 * 4);

    QCOMPARE(NetworkUtil::get4BytesSigned(array, 0), qint32(-1));
    QCOMPARE(NetworkUtil::get4BytesSigned(array, 4), qint32(-2));
    QCOMPARE(NetworkUtil::get4BytesSigned(array, 8), qint32(5));
}

void TestNetworkUtil::get8BytesSigned() {
    QByteArray array;
    array.append('\xFF');
    array.append('\xFF');
    array.append('\xFF');
    array.append('\xFF');
    array.append('\xFF');
    array.append('\xFF');
    array.append('\xFF');
    array.append('\xFF');

    array.append('\xFF');
    array.append('\xFF');
    array.append('\xFF');
    array.append('\xFF');
    array.append('\xFF');
    array.append('\xFF');
    array.append('\xFF');
    array.append('\xFE');

    array.append('\0');
    array.append('\0');
    array.append('\0');
    array.append('\0');
    array.append('\0');
    array.append('\0');
    array.append('\0');
    array.append('\x05');

    QCOMPARE(array.size(), 3 * 8);

    QCOMPARE(NetworkUtil::get8BytesSigned(array, 0), qint64(-1));
    QCOMPARE(NetworkUtil::get8BytesSigned(array, 8), qint64(-2));
    QCOMPARE(NetworkUtil::get8BytesSigned(array, 16), qint64(5));
}

void TestNetworkUtil::getByteUnsignedToInt() {
    QByteArray array;
    array.append('\xFF');
    array.append('\0');
    array.append(char(30));

    QCOMPARE(array.size(), 3 * 1);

    QCOMPARE(NetworkUtil::getByteUnsignedToInt(array, 0), 255);
    QCOMPARE(NetworkUtil::getByteUnsignedToInt(array, 1), 0);
    QCOMPARE(NetworkUtil::getByteUnsignedToInt(array, 2), 30);
}

void TestNetworkUtil::get2BytesUnsignedToInt() {
    QByteArray array;
    array.append('\xFF');
    array.append('\xFF');

    array.append('\0');
    array.append(char(30));

    QCOMPARE(array.size(), 2 * 2);

    QCOMPARE(NetworkUtil::get2BytesUnsignedToInt(array, 0), 65535);
    QCOMPARE(NetworkUtil::get2BytesUnsignedToInt(array, 2), 30);
}

void TestNetworkUtil::appendByteUnsigned() {
    QByteArray array;
    NetworkUtil::appendByteUnsigned(array, 255);
    NetworkUtil::appendByteUnsigned(array, 128);
    NetworkUtil::appendByteUnsigned(array, 33);
    NetworkUtil::appendByteUnsigned(array, 0);

    QCOMPARE(array.size(), 4 * 1);

    QCOMPARE(char(array[0]), '\xFF');
    QCOMPARE(char(array[1]), char(128));
    QCOMPARE(char(array[2]), char(33));
    QCOMPARE(char(array[3]), char(0));
}

void TestNetworkUtil::append2BytesUnsigned() {
    QByteArray array;
    NetworkUtil::append2BytesUnsigned(array, 0xFFFF);
    NetworkUtil::append2BytesUnsigned(array, 0xFF07);
    NetworkUtil::append2BytesUnsigned(array, 256);
    NetworkUtil::append2BytesUnsigned(array, 0);

    QCOMPARE(array.size(), 4 * 2);

    QCOMPARE(char(array[0]), '\xFF');
    QCOMPARE(char(array[1]), '\xFF');
    QCOMPARE(char(array[2]), '\xFF');
    QCOMPARE(char(array[3]), '\x07');
    QCOMPARE(char(array[4]), char(1));
    QCOMPARE(char(array[5]), char(0));
    QCOMPARE(char(array[6]), char(0));
    QCOMPARE(char(array[7]), char(0));
}

void TestNetworkUtil::append2BytesSigned() {
    QByteArray array;
    NetworkUtil::append2BytesSigned(array, qint16(-1));
    NetworkUtil::append2BytesSigned(array, qint16(-1000));

    QCOMPARE(array.size(), 2 * 2);

    QCOMPARE(char(array[0]), '\xFF');
    QCOMPARE(char(array[1]), '\xFF');

    QCOMPARE(char(array[2]), '\xFC');
    QCOMPARE(char(array[3]), '\x18');
}

void TestNetworkUtil::append4BytesSigned() {
    QByteArray array;
    NetworkUtil::append4BytesSigned(array, qint32(-1));
    NetworkUtil::append4BytesSigned(array, qint32(-1000));
    NetworkUtil::append4BytesSigned(array, qint32(-1000000000));

    QCOMPARE(array.size(), 3 * 4);

    QCOMPARE(char(array[0]), '\xFF');
    QCOMPARE(char(array[1]), '\xFF');
    QCOMPARE(char(array[2]), '\xFF');
    QCOMPARE(char(array[3]), '\xFF');

    QCOMPARE(char(array[4]), '\xFF');
    QCOMPARE(char(array[5]), '\xFF');
    QCOMPARE(char(array[6]), '\xFC');
    QCOMPARE(char(array[7]), '\x18');

    QCOMPARE(char(array[8]), '\xC4');
    QCOMPARE(char(array[9]), '\x65');
    QCOMPARE(char(array[10]), '\x36');
    QCOMPARE(char(array[11]), '\x00');
}

void TestNetworkUtil::append8BytesSigned() {
    QByteArray array;
    NetworkUtil::append8BytesSigned(array, qint64(-1));
    NetworkUtil::append8BytesSigned(array, qint64(-1000));
    NetworkUtil::append8BytesSigned(array, qint64(-1000000000));
    NetworkUtil::append8BytesSigned(array, qint64(-1000000000000LL));
    NetworkUtil::append8BytesSigned(array, qint64(-1000000000000001LL));
    NetworkUtil::append8BytesSigned(array, qint64(-100000000000559010LL));

    QCOMPARE(array.size(), 6 * 8);

    QCOMPARE(char(array[0]), '\xFF');
    QCOMPARE(char(array[1]), '\xFF');
    QCOMPARE(char(array[2]), '\xFF');
    QCOMPARE(char(array[3]), '\xFF');
    QCOMPARE(char(array[4]), '\xFF');
    QCOMPARE(char(array[5]), '\xFF');
    QCOMPARE(char(array[6]), '\xFF');
    QCOMPARE(char(array[7]), '\xFF');

    QCOMPARE(char(array[8]), '\xFF');
    QCOMPARE(char(array[9]), '\xFF');
    QCOMPARE(char(array[10]), '\xFF');
    QCOMPARE(char(array[11]), '\xFF');
    QCOMPARE(char(array[12]), '\xFF');
    QCOMPARE(char(array[13]), '\xFF');
    QCOMPARE(char(array[14]), '\xFC');
    QCOMPARE(char(array[15]), '\x18');

    QCOMPARE(char(array[16]), '\xFF');
    QCOMPARE(char(array[17]), '\xFF');
    QCOMPARE(char(array[18]), '\xFF');
    QCOMPARE(char(array[19]), '\xFF');
    QCOMPARE(char(array[20]), '\xC4');
    QCOMPARE(char(array[21]), '\x65');
    QCOMPARE(char(array[22]), '\x36');
    QCOMPARE(char(array[23]), '\x00');

    QCOMPARE(char(array[24]), '\xFF');
    QCOMPARE(char(array[25]), '\xFF');
    QCOMPARE(char(array[26]), '\xFF');
    QCOMPARE(char(array[27]), '\x17');
    QCOMPARE(char(array[28]), '\x2B');
    QCOMPARE(char(array[29]), '\x5A');
    QCOMPARE(char(array[30]), '\xF0');
    QCOMPARE(char(array[31]), '\x00');

    QCOMPARE(char(array[32]), '\xFF');
    QCOMPARE(char(array[33]), '\xFC');
    QCOMPARE(char(array[34]), '\x72');
    QCOMPARE(char(array[35]), '\x81');
    QCOMPARE(char(array[36]), '\x5B');
    QCOMPARE(char(array[37]), '\x39');
    QCOMPARE(char(array[38]), '\x7F');
    QCOMPARE(char(array[39]), '\xFF');

    QCOMPARE(char(array[40]), '\xFE');
    QCOMPARE(char(array[41]), '\x9C');
    QCOMPARE(char(array[42]), '\xBA');
    QCOMPARE(char(array[43]), '\x87');
    QCOMPARE(char(array[44]), '\xA2');
    QCOMPARE(char(array[45]), '\x6D');
    QCOMPARE(char(array[46]), '\x78');
    QCOMPARE(char(array[47]), '\x5E');
}

void TestNetworkUtil::getUtf8String() {
    QByteArray array;
    array.append('p');
    array.append('i');
    array.append('z');
    array.append('z');
    array.append('a');

    QCOMPARE(NetworkUtil::getUtf8String(array, 0, 5), QString("pizza"));
    QCOMPARE(NetworkUtil::getUtf8String(array, 0, 2), QString("pi"));

    //array.clear();
    // TODO : test with non-ascii characters
}

QTEST_MAIN(TestNetworkUtil)
