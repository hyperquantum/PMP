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

#include "obfuscator.h"

#include <QDateTime>

namespace PMP
{
    Obfuscator::Obfuscator(quint64 key)
     : _key(key)
    {
        _randomByte = quint64(QDateTime::currentMSecsSinceEpoch()) & 0xFFu;

        _keyArray[0] = (key >> 56) & 0xFFu;
        _keyArray[1] = (key >> 48) & 0xFFu;
        _keyArray[2] = (key >> 40) & 0xFFu;
        _keyArray[3] = (key >> 32) & 0xFFu;
        _keyArray[4] = (key >> 24) & 0xFFu;
        _keyArray[5] = (key >> 16) & 0xFFu;
        _keyArray[6] = (key >> 8) & 0xFFu;
        _keyArray[7] = key & 0xFFu;
    }

    void Obfuscator::setRandomByte(quint8 value)
    {
        _randomByte = value;
    }

    QByteArray Obfuscator::encrypt(QByteArray input) const
    {
        QByteArray bytes = char(_randomByte) + input;

        for(int i = 0; i < bytes.size(); ++i)
        {
            quint8 count = 1 + (i % 7); // use 7, not 8
            uint value = uchar(bytes[i]);
            uint shifted = value >> count | value << (8 - count);
            bytes[i] = char(shifted & 0xFFu);
        }

        char lastByte = 77;
        for(int i = 0; i < bytes.size(); ++i)
        {
            char keyByte = _keyArray[i % 8];

            bytes[i] = bytes[i] ^ lastByte ^ keyByte;
            lastByte = bytes[i];
        }

        return bytes;
    }

    QByteArray Obfuscator::decrypt(QByteArray input) const
    {
        QByteArray bytes = input;

        char lastByte = 77;
        for(int i = 0; i < bytes.size(); ++i)
        {
            char keyByte = _keyArray[i % 8];

            char beforeDecryption = bytes[i];
            bytes[i] = bytes[i] ^ lastByte ^ keyByte;
            lastByte = beforeDecryption;
        }

        for(int i = 0; i < bytes.size(); ++i)
        {
            quint8 count = 1 + (i % 7); // use 7, not 8
            uint value = uchar(bytes[i]);
            uint shifted = value << count | value >> (8 - count);
            bytes[i] = char(shifted & 0xFFu);
        }

        bytes = bytes.mid(1);

        return bytes;
    }
}
