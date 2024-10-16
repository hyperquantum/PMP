/*
    Copyright (C) 2019-2024, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_TESTNETWORKUTIL_H
#define PMP_TESTNETWORKUTIL_H

#include <QObject>

class TestNetworkUtil : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void fitsIn2BytesSigned();
    void to2BytesSigned();
    void appendByte();
    void getByte();
    void append2Bytes();
    void get2Bytes();
    void append4Bytes();
    void get4Bytes();
    void append8Bytes();
    void get8Bytes();
    void get2BytesSigned();
    void get4BytesSigned();
    void get8BytesSigned();
    void getByteUnsignedToInt();
    void get2BytesUnsignedToInt();
    void appendByteUnsigned();
    void append2BytesUnsigned();
    void append2BytesSigned();
    void append4BytesSigned();
    void append8BytesSigned();
    void getUtf8String();
};
#endif
