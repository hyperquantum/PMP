/*
    Copyright (C) 2014-2020, Kevin Andre <hyperquantum@gmail.com>

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

#include <QCryptographicHash>

namespace PMP {

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

    QString FileHash::toString()
    {
        if (isNull())
            return "(null)";

        return QString::number(length()) + "-" + SHA1().toHex() + "-" + MD5().toHex();
    }

    QString FileHash::dumpToString() const {
        if (isNull()) return "(null)";

        return "(" + QString::number(_length) + "; "
            + _sha1.toHex() + "; "
            + _md5.toHex() + ")";
    }

}
