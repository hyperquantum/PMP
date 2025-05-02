/*
    Copyright (C) 2014-2025, Kevin André <hyperquantum@gmail.com>

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
#include <QtDebug>

#include <compare>

namespace PMP
{
    class FileHash
    {
    public:
        FileHash() : _length(0) { }
        FileHash(uint length, const QByteArray& sha1, const QByteArray& md5);

        static FileHash create(const QByteArray& dataToHash);

        bool isNull() const
        {
            return _length == 0 && _sha1.size() == 0 && _md5.size() == 0;
        }

        uint length() const { return _length; }
        const QByteArray& SHA1() const { return _sha1; }
        const QByteArray& MD5() const { return _md5; }

        QString toString() const;
        QString toFancyString() const;
        QString dumpToString() const;

        static FileHash tryParse(QString const& text);

    private:
        uint _length;
        QByteArray _sha1;
        QByteArray _md5;
    };

    inline std::strong_ordering operator<=>(const FileHash& me, const FileHash& other)
    {
        auto lengthComparison = (me.length() <=> other.length());

        if (lengthComparison != 0)
            return lengthComparison;

        auto sha1Comparison = (me.SHA1() <=> other.SHA1());

        if (sha1Comparison != 0)
            return sha1Comparison;

        auto md5Comparison = (me.MD5() <=> other.MD5());
        return md5Comparison;
    }

    inline bool operator==(const FileHash& me, const FileHash& other)
    {
        return me.length() == other.length()
            && me.SHA1() == other.SHA1()
            && me.MD5() == other.MD5();
    }

    inline size_t qHash(const FileHash& hashID, size_t seed)
    {
        return qHashMulti(seed, hashID.length(), hashID.SHA1(), hashID.MD5());
    }

    inline int compare(const FileHash& me, const FileHash& other)
    {
        auto comparison = me <=> other;
        if (comparison == std::strong_ordering::less) return -1;
        if (comparison == std::strong_ordering::greater) return 1;
        return 0;
    }

    inline QDebug operator<<(QDebug debug, const FileHash& hash)
    {
        if (hash.isNull())
        {
            debug << "(null hash)";
            return debug;
        }

        QString hashText =
                QString::number(hash.length())
                    + "-" + hash.SHA1().toHex()
                    + "-" + hash.MD5().toHex();

        debug << hashText;
        return debug;
    }
}

Q_DECLARE_METATYPE(PMP::FileHash)

#endif
