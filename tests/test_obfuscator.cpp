/*
    Copyright (C) 2020-2022, Kevin Andre <hyperquantum@gmail.com>

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

#include "test_obfuscator.h"

#include "common/obfuscator.h"

#include <QString>
#include <QtTest/QTest>

#include <algorithm>

using namespace PMP;

TestObfuscator::TestObfuscator()
{
    _keys.append(0xF85145DB00A050E6ull);
    _keys.append(0x9dde5e1e45bf2831ull);
    _keys.append(0xb0ce551ba9307379ull);
    _keys.append(0x2213a701bcbafe6aull);
    _keys.append(0xa2f23727dbcba15eull);
    _keys.append(0x7a385a889d3da6fdull);
    _keys.append(0x79766a9d00b2ed20ull);
    _keys.append(0xfd8fd4428541565eull);
    _keys.append(0x33c94d543516fedeull);
    _keys.append(0x04f8bc857d37dcc3ull);
    _keys.append(0x15f78213960c7ae4ull);
    _keys.append(0x3fdc464cf62add56ull);
    _keys.append(0x79d4a54d4f126916ull);
    _keys.append(0x328712fe21612dd7ull);
    _keys.append(0x1d27b8c227d42481ull);

    _randomBytes.append(0xe9u);
    _randomBytes.append(0x1eu);
    _randomBytes.append(0xc2u);
    _randomBytes.append(0x7eu);
    _randomBytes.append(0x56u);
    _randomBytes.append(0x11u);
    _randomBytes.append(0x45u);
    _randomBytes.append(0xceu);
    _randomBytes.append(0x00u);
    _randomBytes.append(0xffu);
    _randomBytes.append(0xeau);
    _randomBytes.append(0x3fu);

    _inputs.append(QString("It's a beautiful day").toUtf8());
    _inputs.append(QByteArray());
    _inputs.append(QString("8527419630123456789875421986532748596142536").toUtf8());
    _inputs.append(QString("bytes").toUtf8());
    _inputs.append(QString("byTes").toUtf8());
    _inputs.append(QString("BYTES").toUtf8());
    _inputs.append(QString("ByTES").toUtf8());
    _inputs.append(QString("ByTeS").toUtf8());
    _inputs.append(QString("ByTEs").toUtf8());
    _inputs.append(QString("BYteS").toUtf8());
    _inputs.append(
        QString(
            "This is a very long string. This is a very long string. This is a very long "
            "string. This is a very long string. This is a very long string. This is a "
            "very long string. This is a very long string. This is a very long string.\n"
            "This is a very long string. This is a very long string. This is a very long "
            "string. This is a very long string. This is a very long string. This is a "
            "very long string. This is a very long string. This is a very long string. "
            "This is a very long string. This is a very long string. This is a very long "
            "string. This is a very long string. This is a very long string. This is a "
            "very long string. This is a very long string. This is a very long string. "
            "This is a very long string. This is a very long string. This is a very long "
            "string. This is a very long string. This is a very long string. This is a "
            "very long string. This is a very long string. This is a very long string.\n"
            "This is a very long string. This is a very long string. This is a very long "
            "string. This is a very long string. This is a very long string. This is a "
            "very long string. This is a very long string. This is a very long string..."
        ).toUtf8()
    );
}

void TestObfuscator::roundtrip()
{
    for (quint8 randomByte : _randomBytes)
    {
        for (quint64 key : _keys)
        {
            for (QByteArray input : _inputs)
            {

                Obfuscator encryptingObfuscator(key);
                encryptingObfuscator.setRandomByte(randomByte);

                QByteArray afterEncryption = encryptingObfuscator.encrypt(input);

                Obfuscator decryptingObfuscator(key);
                decryptingObfuscator.setRandomByte(randomByte);

                QByteArray afterDecryption =
                        decryptingObfuscator.decrypt(afterEncryption);

                QCOMPARE(afterDecryption, input);
            }
        }
    }
}

void TestObfuscator::notNoOp()
{
    for (quint8 randomByte : _randomBytes)
    {
        for (quint64 key : _keys)
        {
            for (QByteArray input : _inputs)
            {

                Obfuscator encryptingObfuscator(key);
                encryptingObfuscator.setRandomByte(randomByte);

                QByteArray afterEncryption = encryptingObfuscator.encrypt(input);

                QVERIFY(afterEncryption != input);

                if (input.size() > 0)
                    QVERIFY(!afterEncryption.contains(input));
            }
        }
    }
}

void TestObfuscator::reproducible()
{
    {
        quint64 key = 0xfd8fd4428541565eull;
        quint8 randomByte = 0x52u;
        Obfuscator o(key);
        o.setRandomByte(randomByte);
        QCOMPARE(
            o.encrypt(QString("RedGreenBlue").toUtf8()),
            QByteArray("\x99\x82\xFA\xFE""A\xC9U\xB9\xDF\x18\n\xE3\xF3")
        );
    }

    {
        quint64 key = 0xa2f23727dbcba15eull;
        quint8 randomByte = 0x79u;
        Obfuscator o(key);
        o.setRandomByte(randomByte);
        QCOMPARE(
            o.encrypt(QString("123456").toUtf8()),
            QByteArray("S\xED\x9C\x88\xF2\xED ")
        );
    }

    {
        quint64 key = 0x3fdc464cf62add56ull;
        quint8 randomByte = 0xFBu;
        Obfuscator o(key);
        o.setRandomByte(randomByte);
        QCOMPARE(
            o.encrypt(QString("GPLv3").toUtf8()),
            QByteArray("\x8F\x82\xCE""F\x03\xE5")
        );
    }

    {
        quint64 key = 0x328712fe21612dd7ull;
        quint8 randomByte = 0xE1u;
        Obfuscator o(key);
        o.setRandomByte(randomByte);
        QCOMPARE(
            o.encrypt(QString("\nNewlines\r\n").toUtf8()),
            QByteArray("\x8F\x8AQ\xF9""c\xB3L\xAC\xC7.\xEC""B")
        );
    }

    {
        quint64 key = 0xb0ce551ba9307379ull;
        quint8 randomByte = 0;
        Obfuscator o(key);
        o.setRandomByte(randomByte);
        QCOMPARE(
            o.encrypt(QString("AAAAAAAAAAAAAAAA").toUtf8()),
            QByteArray("\xFD""c\x1E\x11\xB2\x87v\xAFO\xA9\xE8\xF9U\xE7""4\x1D\x85")
        );
    }

}

void TestObfuscator::randomByteMatters()
{
    quint64 key = 0x15f78213960c7ae4ull;
    QByteArray input = QString("abcdefghij").toUtf8();

    auto randomBytes = _randomBytes;

    // verify that the random bytes are unique
    std::sort(randomBytes.begin(), randomBytes.end());
    for(int i = 1; i < randomBytes.size(); ++i)
    {
        QVERIFY(randomBytes[i - 1] != randomBytes[i]);
    }

    QVector<QByteArray> results;
    for (quint8 randomByte : randomBytes)
    {
        Obfuscator o(key);
        o.setRandomByte(randomByte);
        results.append(o.encrypt(input).mid(1));
    }

    // verify that the encrypted strings are unique
    std::sort(results.begin(), results.end());
    for(int i = 1; i < results.size(); ++i)
    {
        QVERIFY(results[i - 1] != results[i]);
    }
}

void TestObfuscator::keyMatters()
{
    quint8 randomByte = 0x72u;
    QByteArray input = QString("abcdefghij").toUtf8();

    QVector<quint64> keys;
    keys << 0x1234567812345678ull
         << 0x0234567812345678ull
         << 0x1034567812345678ull
         << 0x1204567812345678ull
         << 0x1230567812345678ull
         << 0x1234067812345678ull
         << 0x1234507812345678ull
         << 0x1234560812345678ull
         << 0x1234567012345678ull
         << 0x1234567802345678ull
         << 0x1234567810345678ull
         << 0x1234567812045678ull
         << 0x1234567812305678ull
         << 0x1234567812340678ull
         << 0x1234567812345078ull
         << 0x1234567812345608ull
         << 0x1234567812345670ull;

    // verify that the keys are unique
    std::sort(keys.begin(), keys.end());
    for(int i = 1; i < keys.size(); ++i)
    {
        QVERIFY(keys[i - 1] != keys[i]);
    }

    QVector<QByteArray> results;
    for (quint64 key : keys)
    {
        Obfuscator o(key);
        o.setRandomByte(randomByte);
        results.append(o.encrypt(input));
    }

    // verify that the encrypted strings are unique
    std::sort(results.begin(), results.end());
    for(int i = 1; i < results.size(); ++i)
    {
        QVERIFY(results[i - 1] != results[i]);
    }
}

QTEST_MAIN(TestObfuscator)
