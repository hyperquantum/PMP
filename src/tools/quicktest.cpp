/*
    Copyright (C) 2016, Kevin Andre <hyperquantum@gmail.com>

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

#include "common/util.h"

#include <QHostAddress>
#include <QHostInfo>
#include <QNetworkInterface>
#include <QtDebug>

int main(int argc, char *argv[]) {
    /*
    int seed;
    seed = PMP::Util::getRandomSeed();
    seed = PMP::Util::getRandomSeed();
    seed = PMP::Util::getRandomSeed();
    qDebug << "last seed:" << seed;
    */

    qDebug() << "Local hostname:" << QHostInfo::localHostName();

    foreach (const QHostAddress& address, QNetworkInterface::allAddresses()) {
        qDebug() << address.toString();
    }

    return 0;
}
