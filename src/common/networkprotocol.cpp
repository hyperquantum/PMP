/*
    Copyright (C) 2015, Kevin Andre <hyperquantum@gmail.com>

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

#include "networkprotocol.h"

#include <QCryptographicHash>

namespace PMP {

    int NetworkProtocol::ratePassword(QString password) {
        int rating = 0;
        for (int i = 0; i < password.size(); ++i) {
            rating += 3;
            QChar c = password[i];

            if (c.isDigit()){
                rating += 1; /* digits are slightly better than lowercase letters */
            }
            else if (c.isLetter()){
                if (c.isLower()) {
                    /* lowercase letters worth the least */
                }
                else {
                    rating += 2; /* uppercase letters are better than digits */
                }
            }
            else {
                rating += 7;
            }
        }

        int lastDiff = 0;
        int diffConstantCount = 0;
        for (int i = 1; i < password.size(); ++i) {
            QChar prevC = password[i - 1];
            int prevN = prevC.unicode();
            QChar curC = password[i];
            int curN = curC.unicode();

            /* punish patterns such as "eeeee", "123456", "98765", "ghijklm" etc... */
            int diff = curN - prevN;
            if (diff <= 1 && diff >= -1) {
                rating -= 1;
            }
            if (diff == lastDiff) {
                diffConstantCount += 1;
                rating -= diffConstantCount;
            }
            else {
                diffConstantCount = 0;
            }

            lastDiff = diff;
        }

        return rating;
    }

    QByteArray NetworkProtocol::hashPassword(QByteArray const& salt, QString password) {
        QCryptographicHash hasher(QCryptographicHash::Sha256);
        hasher.addData(salt);
        hasher.addData(password.toUtf8());
        return hasher.result();
    }

}
