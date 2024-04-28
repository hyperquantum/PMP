/*
    Copyright (C) 2014-2024, Kevin Andre <hyperquantum@gmail.com>

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

#include "filehash.h"

#include "unicodechars.h"

#include <QCryptographicHash>

namespace PMP
{
    namespace
    {
        bool isHexEncoded(const QByteArray& bytes)
        {
            if (bytes.length() % 2 != 0)
                return false;

            for (int i = 0; i < bytes.length(); ++i)
            {
                switch (bytes.at(i))
                {
                case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
                case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
                case '0': case '1': case '2': case '3': case '4':
                case '5': case '6': case '7': case '8': case '9':
                    continue; /* valid character */

                default:
                    return false; /* invalid character */
                }
            }

            return true;
        }

        QByteArray tryDecodeHexWithExpectedLength(const QString& text, int expectedLength)
        {
            if (text.length() != expectedLength)
                return {};

            QByteArray hex = text.toLatin1();

            /* check again (non-latin1 chars may have been removed) */
            if (hex.length() != expectedLength)
                return {};

            if (!isHexEncoded(hex))
                return {};

            QByteArray decoded = QByteArray::fromHex(hex);
            if (decoded.length() * 2 != expectedLength)
                return {};

            return decoded;
        }

        FileHash tryParseFileHashInternal(const QString& text)
        {
            const auto parts = text.split(QChar('-'), Qt::KeepEmptyParts);
            if (parts.size() != 3)
                return {};

            bool ok;
            uint length = parts[0].toUInt(&ok);
            if (!ok || length == 0)
                return {};

            QByteArray sha1 = tryDecodeHexWithExpectedLength(parts[1], 40);
            if (sha1.isEmpty())
                return {};

            QByteArray md5 = tryDecodeHexWithExpectedLength(parts[2], 32);
            if (md5.isEmpty())
                return {};

            return FileHash(length, sha1, md5);
        }

        QString fileHashToStringInternal(FileHash const& hash, QChar dash)
        {
            if (hash.isNull())
                return "(null)";

            return QString::number(hash.length())
                   + dash + hash.SHA1().toHex()
                   + dash + hash.MD5().toHex();
        }
    }

    FileHash::FileHash(uint length, const QByteArray& sha1,
        const QByteArray& md5)
     : _length(length), _sha1(sha1), _md5(md5)
    {
        //
    }

    FileHash FileHash::create(const QByteArray& dataToHash)
    {
        auto size = dataToHash.size();

        QCryptographicHash sha1Hasher(QCryptographicHash::Sha1);
        sha1Hasher.addData(dataToHash);
        auto sha1 = sha1Hasher.result();

        QCryptographicHash md5Hasher(QCryptographicHash::Md5);
        md5Hasher.addData(dataToHash);
        auto md5 = md5Hasher.result();

        return FileHash(size, sha1, md5);
    }

    QString FileHash::toString() const
    {
        auto dash = '-';
        return fileHashToStringInternal(*this, dash);
    }

    QString FileHash::toFancyString() const
    {
        return fileHashToStringInternal(*this, UnicodeChars::figureDash);
    }

    QString FileHash::dumpToString() const
    {
        if (isNull())
            return "(null)";

        return "(" + QString::number(_length) + "; "
            + _sha1.toHex() + "; "
            + _md5.toHex() + ")";
    }

    FileHash FileHash::tryParse(const QString& text)
    {
        auto simplifiedText = text;
        simplifiedText.replace(UnicodeChars::figureDash, '-');

        return tryParseFileHashInternal(simplifiedText);
    }
}
