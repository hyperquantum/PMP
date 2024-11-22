/*
    Copyright (C) 2024, Kevin Andre <hyperquantum@gmail.com>

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

#include "test_passwordstrengthevaluator.h"

#include "common/passwordstrengthevaluator.h"

#include <QtTest/QTest>
#include <QVector>

using namespace PMP;

void TestPasswordStrengthEvaluator::passwordsThatAreTooWeak()
{
    const QVector<QString> weakPasswords
        {
            "test",
            "iamweak",
            "password",
            "password123",
            "123456789",
            "abcdefghijklm",
            "hunter2",
            "happy!",
            "MyMusic",
        };

    for (auto& weakPassword : weakPasswords)
    {
        auto rating = PasswordStrengthEvaluator::getPasswordRating(weakPassword);
        QCOMPARE(rating, PasswordStrengthRating::TooWeak);
    }
}

void TestPasswordStrengthEvaluator::passwordsThatAreNotWeak()
{
    const QVector<QString> passwordsNotWeak
        {
            "Rue4cG&mhKd",
            "90846703458154698273",
        };

    for (auto& acceptablePassword : passwordsNotWeak)
    {
        auto rating = PasswordStrengthEvaluator::getPasswordRating(acceptablePassword);
        QVERIFY(rating != PasswordStrengthRating::TooWeak);
    }
}

QTEST_MAIN(TestPasswordStrengthEvaluator)
