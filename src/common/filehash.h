/*
    Copyright (C) 2014-2016, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_HASHID_H
#define PMP_HASHID_H

#include <QByteArray>
#include <QHash>
#include <QMetaType>
#include <QString>

namespace PMP {

    class FileHash {
    public:
        FileHash();
        FileHash(uint length, const QByteArray& sha1,
            const QByteArray& md5);

        bool empty() const {
            return _length == 0 && _sha1.size() == 0 && _md5.size() == 0;
        }

        uint length() const { return _length; }
        const QByteArray& SHA1() const { return _sha1; }
        const QByteArray& MD5() const { return _md5; }

        QString dumpToString() const;

    private:
        uint _length;
        QByteArray _sha1;
        QByteArray _md5;
    };

    inline bool operator==(const FileHash& me, const FileHash& other) {
        return me.length() == other.length()
            && me.SHA1() == other.SHA1()
            && me.MD5() == other.MD5();
    }

    inline bool operator!=(const FileHash& me, const FileHash& other) {
        return !(me == other);
    }

    inline uint qHash(const FileHash& hashID) {
        return hashID.length()
            ^ qHash(hashID.SHA1())
            ^ qHash(hashID.MD5());
    }

    inline int compare(const FileHash& me, const FileHash& other) {
        if (me.length() < other.length()) return -1;
        if (other.length() < me.length()) return 1;

        if (me.SHA1() < other.SHA1()) return -1;
        if (other.SHA1() < me.SHA1()) return 1;

        if (me.MD5() < other.MD5()) return -1;
        if (other.MD5() < me.MD5()) return 1;

        return 0;
    }

    inline bool operator<(const FileHash& me, const FileHash& other) {
        return compare(me, other) < 0;
    }

    inline bool operator<=(const FileHash& me, const FileHash& other) {
        return compare(me, other) <= 0;
    }

    inline bool operator>(const FileHash& me, const FileHash& other) {
        return compare(me, other) > 0;
    }

    inline bool operator>=(const FileHash& me, const FileHash& other) {
        return compare(me, other) >= 0;
    }
}

Q_DECLARE_METATYPE(PMP::FileHash)

#endif
