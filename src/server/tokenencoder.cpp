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

#include "tokenencoder.h"

#include "common/obfuscator.h"

#include <QRandomGenerator>
#include <QtDebug>

namespace PMP
{
    bool TokenEncoder::ensureIsEncoded(QString& token)
    {
        if (token.isEmpty() || token.startsWith('?'))
            return false;

        token = encodeToken(token);
        return true;
    }

    QString TokenEncoder::encodeToken(QString token)
    {
        // minus 1 because of the string NUL terminator
        auto charsCount = (int)sizeof(_keyChars) - 1;

        auto keyCharIndex = QRandomGenerator::global()->bounded(charsCount);
        auto keyChar = _keyChars[keyCharIndex];
        auto key = getKey(keyChar);

        if (key == 0)
        {
            qWarning() << "failed to select a key for encoding the token";
            return {};
        }

        Obfuscator obfuscator(key);
        auto encrypted = obfuscator.encrypt(token.toUtf8());

        return QString("?") + keyChar + QString::fromLatin1(encrypted.toBase64());
    }

    QString TokenEncoder::decodeToken(QString token)
    {
        if (token.isEmpty())
            return {};

        if (!token.startsWith('?'))
            return token; /* token in plain text */

        if (token.length() < 2)
        {
            qWarning() << "cannot decode invalid token";
            return {}; /* invalid token */
        }

        auto keyChar = token[1];
        auto key = getKey(keyChar.toLatin1());
        if (key == 0)
        {
            qWarning() << "could not determine which key to use for decrypting token";
            return {};
        }

        Obfuscator obfuscator(key);
        auto encrypted = QByteArray::fromBase64(token.mid(2).toLatin1());
        auto decryptedUtf8 = obfuscator.decrypt(encrypted);
        auto decrypted = QString::fromUtf8(decryptedUtf8);

        return decrypted;
    }

    quint64 TokenEncoder::getKey(char keyChar)
    {
        switch (keyChar)
        {
        case '0': return 0x2dae592ab2a5753bULL;
        case '2': return 0xc8206a3082b15a6cULL;
        case '4': return 0xb015330ffe44bc95ULL;
        case '6': return 0xa57c7d820bc68d80ULL;
        case '9': return 0xda97fb641b0f7c11ULL;
        case 'A': return 0x1733e6b241926dcfULL;
        case 'b': return 0xb2495c6128755e4cULL;
        case 'c': return 0x90fc13a0af1b7366ULL;
        case 'f': return 0x1e9a141afceb0c28ULL;
        case 'G': return 0x7394856ffad1fa5cULL;
        case 'i': return 0xac4fc3560ab3f478ULL;
        case 'j': return 0x38e17eb0d9405f6aULL;
        case 'm': return 0xd7e74b30dbaa0b30ULL;
        case 'N': return 0x7051119227f394efULL;
        case 'Q': return 0x35eae4c7238cec9eULL;
        case 'r': return 0x9efea1e5e5f7b37bULL;
        case 'S': return 0xc8736facd7bbf026ULL;
        case 't': return 0x8c6f0f4830287042ULL;
        case 'w': return 0x79c0bf6707057978ULL;
        case 'W': return 0x51d0295d8c81b72bULL;
        }

        return 0;
    }
}
