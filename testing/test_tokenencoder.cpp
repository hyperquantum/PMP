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

#include "test_tokenencoder.h"

#include "server/tokenencoder.h"

#include <QtTest/QTest>
#include <QVector>

using namespace PMP;

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
    tokens << "AbCdEfGhIjKlMnOp"
           << "ffddsgfg586151515dsgsdg8451gssg"
           << "cdef0ab32";

    for (auto token : tokens) {
        auto encoded = TokenEncoder::encodeToken(token);
        auto decoded = TokenEncoder::decodeToken(encoded);

        QCOMPARE(decoded, token);
    }
}

QTEST_MAIN(TestTokenEncoder)
