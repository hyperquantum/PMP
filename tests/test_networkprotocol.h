/*
    Copyright (C) 2020-2024, Kevin Andre <hyperquantum@gmail.com>

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

#ifndef PMP_TESTNETWORKPROTOCOL_H
#define PMP_TESTNETWORKPROTOCOL_H

#include <QObject>

class TestNetworkProtocol : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void fileHashByteCount();
    void appendHash();
    void getHash();
    void appendEmptyHash();
    void getEmptyHash();
};
#endif
