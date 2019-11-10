/*
    Copyright (C) 2014-2019, Kevin Andre <hyperquantum@gmail.com>

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
#include <QDateTime>
#include <QString>

namespace PMP {

    class NetworkUtil {
    public:

        static int appendByte(QByteArray& buffer, quint8 b);
        static int append2Bytes(QByteArray& buffer, quint16 number);
        static int append4Bytes(QByteArray& buffer, quint32 number);
        static int append8Bytes(QByteArray& buffer, quint64 number);

        static int append8ByteQDateTimeMsSinceEpoch(QByteArray& buffer,
                                                    QDateTime dateTime);
        static int append8ByteMaybeEmptyQDateTimeMsSinceEpoch(QByteArray& buffer,
                                                              QDateTime dateTime);

        static quint32 get4Bytes(char const* buffer);

        static quint8 getByte(QByteArray const& buffer, uint position);
        static quint16 get2Bytes(QByteArray const& buffer, uint position);
        static quint32 get4Bytes(QByteArray const& buffer, uint position);
        static quint64 get8Bytes(QByteArray const& buffer, uint position);

        static QDateTime getQDateTimeFrom8ByteMsSinceEpoch(QByteArray const& buffer,
                                                           uint position);
        static QDateTime getMaybeEmptyQDateTimeFrom8ByteMsSinceEpoch(
                                                           QByteArray const& buffer,
                                                           uint position);

        static QString getUtf8String(QByteArray const& buffer, int position, int length);

    private:
        template<typename T>
        static char extractByte0(T number) {
            return static_cast<char>(number & 0xFFu);
        }

        template<typename T>
        static char extractByte1(T number) {
            return static_cast<char>((number >> 8) & 0xFFu);
        }

        template<typename T>
        static char extractByte2(T number) {
            return static_cast<char>((number >> 16) & 0xFFu);
        }

        template<typename T>
        static char extractByte3(T number) {
            return static_cast<char>((number >> 24) & 0xFFu);
        }

        template<typename T>
        static char extractByte4(T number) {
            return static_cast<char>((number >> 32) & 0xFFu);
        }

        template<typename T>
        static char extractByte5(T number) {
            return static_cast<char>((number >> 40) & 0xFFu);
        }

        template<typename T>
        static char extractByte6(T number) {
            return static_cast<char>((number >> 48) & 0xFFu);
        }

        template<typename T>
        static char extractByte7(T number) {
            return static_cast<char>((number >> 56) & 0xFFu);
        }

        static quint16 compose2Bytes(quint8 b1, quint8 b0);
        static quint32 compose4Bytes(quint8 b3, quint8 b2, quint8 b1, quint8 b0);
        static quint64 compose8Bytes(quint8 b7, quint8 b6, quint8 b5, quint8 b4,
                                     quint8 b3, quint8 b2, quint8 b1, quint8 b0);

        static qint32 asSigned(quint32 number);
        static qint64 asSigned(quint64 number);
    };
}
#endif
