/*
    Copyright (C) 2014-2021, Kevin Andre <hyperquantum@gmail.com>

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

#include "networkutil.h"

#include <limits>

#include <QtDebug>

namespace PMP
{
    int NetworkUtil::fitsIn2BytesSigned(int number)
    {
        return number <= std::numeric_limits<qint16>::max()
                && number >= std::numeric_limits<qint16>::min();
    }

    qint16 NetworkUtil::to2BytesSigned(int number, bool& errorFlag,
                                       const char* whatIsConverted)
    {
        if (fitsIn2BytesSigned(number))
            return static_cast<qint16>(number);

        errorFlag = true;
        qWarning() << whatIsConverted << "value" << number
                   << "does not fit in 16-bit int";
        return 0;
    }

    int NetworkUtil::appendByte(QByteArray& buffer, quint8 b)
    {
        buffer.append(extractByte0(b));
        return 1;
    }

    int NetworkUtil::append2Bytes(QByteArray& buffer, quint16 number)
    {
        buffer.append(extractByte1(number));
        buffer.append(extractByte0(number));
        return 2;
    }

    int NetworkUtil::append4Bytes(QByteArray& buffer, quint32 number)
    {
        buffer.append(extractByte3(number));
        buffer.append(extractByte2(number));
        buffer.append(extractByte1(number));
        buffer.append(extractByte0(number));
        return 4;
    }

    int NetworkUtil::append8Bytes(QByteArray& buffer, quint64 number)
    {
        buffer.append(extractByte7(number));
        buffer.append(extractByte6(number));
        buffer.append(extractByte5(number));
        buffer.append(extractByte4(number));
        buffer.append(extractByte3(number));
        buffer.append(extractByte2(number));
        buffer.append(extractByte1(number));
        buffer.append(extractByte0(number));
        return 8;
    }

    int NetworkUtil::appendByteUnsigned(QByteArray& buffer, int number)
    {
        if (number < 0 || number > 255)
        {
            qWarning() << "cannot convert" << number << "to unsigned byte";
            number = 0;
        }

        return appendByte(buffer, static_cast<quint8>(number));
    }

    int NetworkUtil::append2BytesUnsigned(QByteArray& buffer, int number)
    {
        if (number < 0 || number > 0xFFFF)
        {
            qWarning() << "cannot convert" << number << "to quint16";
            number = 0;
        }

        return append2Bytes(buffer, static_cast<quint16>(number));
    }

    int NetworkUtil::append2BytesSigned(QByteArray& buffer, qint16 number)
    {
        return append2Bytes(buffer, static_cast<quint16>(number));
    }

    int NetworkUtil::append4BytesSigned(QByteArray& buffer, qint32 number)
    {
        return append4Bytes(buffer, static_cast<quint32>(number));
    }

    int NetworkUtil::append8BytesSigned(QByteArray& buffer, qint64 number)
    {
        return append8Bytes(buffer, static_cast<quint64>(number));
    }

    int NetworkUtil::append8ByteQDateTimeMsSinceEpoch(QByteArray& buffer,
                                                      QDateTime dateTime)
    {
        if (!dateTime.isValid())
        {
            qWarning() << "writing invalid QDateTime to buffer";
        }

        qint64 milliSeconds = dateTime.toUTC().toMSecsSinceEpoch();
        return append8Bytes(buffer, static_cast<quint64>(milliSeconds));
    }

    int NetworkUtil::append8ByteMaybeEmptyQDateTimeMsSinceEpoch(QByteArray& buffer,
                                                                QDateTime dateTime)
    {
        if (!dateTime.isValid())
            return append8Bytes(buffer,
                                static_cast<quint64>(std::numeric_limits<qint64>::min()));

        qint64 milliSeconds = dateTime.toUTC().toMSecsSinceEpoch();
        return append8Bytes(buffer, static_cast<quint64>(milliSeconds));
    }

    quint8 NetworkUtil::getByte(QByteArray const& buffer, int position)
    {
        return static_cast<quint8>(buffer[position]);
    }

    quint16 NetworkUtil::get2Bytes(char const* buffer)
    {
        return
            compose2Bytes(
                static_cast<quint8>(buffer[0]),
                static_cast<quint8>(buffer[1])
            );
    }

    quint16 NetworkUtil::get2Bytes(QByteArray const& buffer, int position)
    {
        return
            compose2Bytes(
                static_cast<quint8>(buffer[position]),
                static_cast<quint8>(buffer[position + 1])
            );
    }

    quint32 NetworkUtil::get4Bytes(char const* buffer)
    {
        return
            compose4Bytes(
                static_cast<quint8>(buffer[0]),
                static_cast<quint8>(buffer[1]),
                static_cast<quint8>(buffer[2]),
                static_cast<quint8>(buffer[3])
            );
    }

    quint32 NetworkUtil::get4Bytes(QByteArray const& buffer, int position)
    {
        return
            compose4Bytes(
                static_cast<quint8>(buffer[position]),
                static_cast<quint8>(buffer[position + 1]),
                static_cast<quint8>(buffer[position + 2]),
                static_cast<quint8>(buffer[position + 3])
            );
    }

    quint64 NetworkUtil::get8Bytes(QByteArray const& buffer, int position)
    {
        return
            compose8Bytes(
                static_cast<quint8>(buffer[position]),
                static_cast<quint8>(buffer[position + 1]),
                static_cast<quint8>(buffer[position + 2]),
                static_cast<quint8>(buffer[position + 3]),
                static_cast<quint8>(buffer[position + 4]),
                static_cast<quint8>(buffer[position + 5]),
                static_cast<quint8>(buffer[position + 6]),
                static_cast<quint8>(buffer[position + 7])
            );
    }

    qint16 NetworkUtil::get2BytesSigned(QByteArray const& buffer, int position)
    {
        return static_cast<qint16>(get2Bytes(buffer, position));
    }

    qint32 NetworkUtil::get4BytesSigned(QByteArray const& buffer, int position)
    {
        return static_cast<qint32>(get4Bytes(buffer, position));
    }

    qint64 NetworkUtil::get8BytesSigned(QByteArray const& buffer, int position)
    {
        return static_cast<qint64>(get8Bytes(buffer, position));
    }

    int NetworkUtil::getByteUnsignedToInt(QByteArray const& buffer, int position)
    {
        return static_cast<int>(static_cast<uint>(getByte(buffer, position)));
    }

    int NetworkUtil::get2BytesUnsignedToInt(QByteArray const& buffer, int position)
    {
        return static_cast<int>(static_cast<uint>(get2Bytes(buffer, position)));
    }

    QDateTime NetworkUtil::getQDateTimeFrom8ByteMsSinceEpoch(const QByteArray& buffer,
                                                             int position)
    {
        auto msecs = asSigned(get8Bytes(buffer, position));
        return QDateTime::fromMSecsSinceEpoch(msecs, Qt::UTC);
    }

    QDateTime NetworkUtil::getMaybeEmptyQDateTimeFrom8ByteMsSinceEpoch(
                                                   const QByteArray& buffer, int position)
    {
        auto msecs = asSigned(get8Bytes(buffer, position));
        if (msecs == std::numeric_limits<qint64>::min())
            return QDateTime(); /* return invalid QDateTime */

        return QDateTime::fromMSecsSinceEpoch(msecs, Qt::UTC);
    }

    QString NetworkUtil::getUtf8String(QByteArray const& buffer, int position,
                                       int length)
    {
        if (position < 0)
        {
            qWarning() << "position < 0 !!";
            return "";
        }

        if (length < 0)
        {
            qWarning() << "length < 0 !!";
            return "";
        }

        if (length > buffer.size() || position > buffer.size() - length)
        {
            qWarning() << "OVERFLOW when reading UTF-8 string; position:" << position
                       << "; length:" << length << "; buffer size:" << buffer.size();
            return "";
        }

        return QString::fromUtf8(&buffer.data()[position], length);
    }

    QByteArray NetworkUtil::getUtf8Bytes(QString text, int maxByteCount)
    {
        if (maxByteCount < 0)
        {
            qWarning() << "getUtf8Bytes: maxByteCount < 0 !!";
            return {};
        }

        auto bytes = text.toUtf8();
        if (bytes.size() <= maxByteCount)
            return bytes;

        /* we have to cut off the string, find the right spot */
        int cutOff = maxByteCount;
        for (int i = 0; i < 4; ++i)
        {
            if ((bytes[cutOff] & 0b11000000) != 0b10000000)
            {
                return bytes.left(cutOff);
            }

            --cutOff;
        }

        qWarning() << "getUtf8Bytes: invalid UTF-8 detected!!";
        return {};
    }

    quint16 NetworkUtil::compose2Bytes(quint8 b1, quint8 b0)
    {
        auto sum =
            (static_cast<quint16>(b1) << 8)
            + static_cast<quint16>(b0);

        return static_cast<quint16>(sum);
    }

    quint32 NetworkUtil::compose4Bytes(quint8 b3, quint8 b2, quint8 b1, quint8 b0)
    {
        auto sum =
            (static_cast<quint32>(b3) << 24)
            + (static_cast<quint32>(b2) << 16)
            + (static_cast<quint32>(b1) << 8)
            + static_cast<quint32>(b0);

        return static_cast<quint32>(sum);
    }

    quint64 NetworkUtil::compose8Bytes(quint8 b7, quint8 b6, quint8 b5, quint8 b4,
                                       quint8 b3, quint8 b2, quint8 b1, quint8 b0)
    {
        auto sum =
            (static_cast<quint64>(b7) << 56)
            + (static_cast<quint64>(b6) << 48)
            + (static_cast<quint64>(b5) << 40)
            + (static_cast<quint64>(b4) << 32)
            + (static_cast<quint64>(b3) << 24)
            + (static_cast<quint64>(b2) << 16)
            + (static_cast<quint64>(b1) << 8)
            + static_cast<quint64>(b0);

        return static_cast<quint64>(sum);
    }

    qint32 NetworkUtil::asSigned(quint32 number)
    {
        return static_cast<qint32>(number);
    }

    qint64 NetworkUtil::asSigned(quint64 number)
    {
        return static_cast<qint64>(number);
    }

}
