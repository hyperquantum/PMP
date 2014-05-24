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

#include "networkutil.h"

#include <QDebug>

namespace PMP {

    int NetworkUtil::appendByte(QByteArray& buffer, quint8 b) {
        buffer.append((char)b);
        return 1;
    }

    int NetworkUtil::append2Bytes(QByteArray& buffer, quint16 number) {
        buffer.append((char)((number >> 8) & 255));
        buffer.append((char)(number & 255));
        return 2;
    }

    int NetworkUtil::append4Bytes(QByteArray& buffer, quint32 number) {
        buffer.append((char)((number >> 24) & 255));
        buffer.append((char)((number >> 16) & 255));
        buffer.append((char)((number >> 8) & 255));
        buffer.append((char)(number & 255));
        return 4;
    }

    int NetworkUtil::append8Bytes(QByteArray& buffer, quint64 number) {
        buffer.append((char)((number >> 56) & 255));
        buffer.append((char)((number >> 48) & 255));
        buffer.append((char)((number >> 40) & 255));
        buffer.append((char)((number >> 32) & 255));
        buffer.append((char)((number >> 24) & 255));
        buffer.append((char)((number >> 16) & 255));
        buffer.append((char)((number >> 8) & 255));
        buffer.append((char)(number & 255));
        return 8;
    }

    quint8 NetworkUtil::getByte(QByteArray const& buffer, uint position) {
        return buffer[position];
    }

    quint16 NetworkUtil::get2Bytes(QByteArray const& buffer, uint position) {
        return ((quint16)(quint8)buffer[position] << 8)
            + (quint16)(quint8)buffer[position + 1];
    }

    quint32 NetworkUtil::get4Bytes(QByteArray const& buffer, uint position) {
        return ((quint32)(quint8)buffer[position] << 24)
            + ((quint32)(quint8)buffer[position + 1] << 16)
            + ((quint32)(quint8)buffer[position + 2] << 8)
            + (quint32)(quint8)buffer[position + 3];
    }

    quint64 NetworkUtil::get8Bytes(QByteArray const& buffer, uint position) {
        return ((quint64)(quint8)buffer[position] << 56)
            + ((quint64)(quint8)buffer[position + 1] << 48)
            + ((quint64)(quint8)buffer[position + 2] << 40)
            + ((quint64)(quint8)buffer[position + 3] << 32)
            + ((quint64)(quint8)buffer[position + 4] << 24)
            + ((quint64)(quint8)buffer[position + 5] << 16)
            + ((quint64)(quint8)buffer[position + 6] << 8)
            + (quint64)(quint8)buffer[position + 7];
    }

    QString NetworkUtil::getUtf8String(QByteArray const& buffer, uint position, uint length) {
        if (length > (uint)buffer.size() || position > (uint)buffer.size() - length) {
            qDebug() << "OVERFLOW in NetworkUtil::getUtf8String; position" << position << " length" << length << " buffer size" << buffer.size();
            return "";
        }

        return QString::fromUtf8(&buffer.data()[position], length);
    }

}
