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

#include "test_tokenencoder.h"

#include "server/tokenencoder.h"

#include <QtTest/QTest>
#include <QVector>

using namespace PMP;
using namespace PMP::Server;

void TestTokenEncoder::ensureIsEncodedDoesNotChangeEmptyString()
{
    QString empty = "";

    auto changed = TokenEncoder::ensureIsEncoded(empty);

    QVERIFY(!changed);
    QCOMPARE(empty, QString());
}

void TestTokenEncoder::ensureIsEncodedDoesNotChangeEncodedString()
{
    QString original = "?2xHWS9WQ=";
    QString text = original;

    auto changed = TokenEncoder::ensureIsEncoded(text);

    QVERIFY(!changed);
    QCOMPARE(text, original);
}

void TestTokenEncoder::ensureIsEncodedChangesPlainTextString()
{
    QString text = "Abcdef123";

    auto changed = TokenEncoder::ensureIsEncoded(text);

    QVERIFY(changed);
    QVERIFY(text.startsWith('?'));
}

void TestTokenEncoder::encodeUsesObfuscation()
{
    QVector<QString> tokens;
    tokens << "~"
           << "*"
           << "+"
           << "unlikely";

    for (auto& token : qAsConst(tokens))
    {
        auto encoded = TokenEncoder::encodeToken(token);

        QVERIFY(encoded.startsWith("?"));
        QVERIFY(!encoded.contains(token));
    }
}

void TestTokenEncoder::decodeEmpty()
{
    QCOMPARE(TokenEncoder::decodeToken(""), QString(""));
}

void TestTokenEncoder::decodePlainText()
{
    QCOMPARE(TokenEncoder::decodeToken("abcdef"), QString("abcdef"));
    QCOMPARE(TokenEncoder::decodeToken("123456789"), QString("123456789"));
}

void TestTokenEncoder::roundtrip()
{
    QVector<QString> tokens;
    tokens << "()"
           << "AbCdEfGhIjKlMnOp"
           << "ffddsgfg586151515dsgsdg8451gssg"
           << "cdef0ab32"
           << "plain hot tomato soup";

    for (auto& token : qAsConst(tokens))
    {
        auto encoded = TokenEncoder::encodeToken(token);
        auto decoded = TokenEncoder::decodeToken(encoded);

        QCOMPARE(decoded, token);
    }
}

QTEST_MAIN(TestTokenEncoder)
