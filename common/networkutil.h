/*
    Copyright (C) 2014, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_NETWORKUTIL_H
#define PMP_NETWORKUTIL_H

#include <QByteArray>
#include <QString>

namespace PMP {

    class NetworkUtil {
    public:

        static int appendByte(QByteArray& buffer, quint8 b);
        static int append2Bytes(QByteArray& buffer, quint16 number);
        static int append4Bytes(QByteArray& buffer, quint32 number);
        static int append8Bytes(QByteArray& buffer, quint64 number);

        static quint8 getByte(QByteArray const& buffer, uint position);
        static quint16 get2Bytes(QByteArray const& buffer, uint position);
        static quint32 get4Bytes(QByteArray const& buffer, uint position);
        static quint64 get8Bytes(QByteArray const& buffer, uint position);
        static QString getUtf8String(QByteArray const& buffer, uint position, uint length);

    };
}
#endif
