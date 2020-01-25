/*
    Copyright (C) 2018-2020, Kevin Andre <hyperquantum@gmail.com>

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

#include "common/logging.h"
#include "common/util.h"
#include "common/version.h"

#include <QCoreApplication>
#include <QHostAddress>
#include <QHostInfo>
#include <QNetworkInterface>
#include <QSslSocket>
#include <QtDebug>

using namespace PMP;

int main(int argc, char *argv[]) {
    
    QCoreApplication app(argc, argv);

    QCoreApplication::setApplicationName("Party Music Player - Simple test program");
    QCoreApplication::setApplicationVersion(PMP_VERSION_DISPLAY);
    QCoreApplication::setOrganizationName(PMP_ORGANIZATION_NAME);
    QCoreApplication::setOrganizationDomain(PMP_ORGANIZATION_DOMAIN);

    /* set up logging */
    Logging::enableConsoleAndTextFileLogging(false);
    Logging::setFilenameTag("T"); /* T = Test */

    /*
    qDebug() << "Local hostname:" << QHostInfo::localHostName();

    foreach (const QHostAddress& address, QNetworkInterface::allAddresses()) {
        qDebug() << address.toString();
    }
    */
    
    qDebug() << "SSL version:" << QSslSocket::sslLibraryVersionNumber();
    qDebug() << "SSL version string:" << QSslSocket::sslLibraryVersionString();
    return 0;
}
