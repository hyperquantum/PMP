/*
    Copyright (C) 2018-2021, Kevin Andre <hyperquantum@gmail.com>

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

#include "server/database.h"
#include "server/lastfmscrobblingbackend.h"

#include <QCoreApplication>
#include <QHostAddress>
#include <QHostInfo>
#include <QNetworkInterface>
#include <QSslSocket>
#include <QtDebug>
#include <QtGlobal>

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

    QTextStream out(stdout);
    qDebug() << "Qt version:" << qVersion();

    /*
    qDebug() << "Local hostname:" << QHostInfo::localHostName();

    foreach (const QHostAddress& address, QNetworkInterface::allAddresses()) {
        qDebug() << address.toString();
    }
    */
    
    /*
    qDebug() << "SSL version:" << QSslSocket::sslLibraryVersionNumber();
    qDebug() << "SSL version string:" << QSslSocket::sslLibraryVersionString();
    */

    //return 0;

    //auto lastFm = new PMP::LastFmScrobblingBackend();

    /*
    QObject::connect(
        lastFm, &PMP::LastFmScrobblingBackend::receivedAuthenticationReply,
        &app, &QCoreApplication::quit
    );*/

    //lastFm->authenticateWithCredentials("xxxxxxxxxxxx", "xxxxxxxxxxxxxxx");

    //lastFm->setSessionKey("xxxxxxxxxxxxx");

    /*
    lastFm->doScrobbleCall(
        QDateTime(QDate(2018, 03, 05), QTime(15, 57), Qt::LocalTime),
        "Title",
        QString("Artist"),
        "Album",
        3 * 60 + 39
    );*/

    //return app.exec();

    if (!Database::init(out, "localhost", "root", "xxxxxxxxxxx"))
    {
        qWarning() << "could not initialize database";
        return 1;
    }

    auto database = Database::getDatabaseForCurrentThread();
    if (!database)
    {
        qWarning() << "could not get database connection for this thread";
        return 1;
    }

    bool ok;
    auto preferences = database->getUserDynamicModePreferences(1, &ok);
    if (!ok)
    {
        qWarning() << "failed to load user preferences";
        return 1;
    }

    qDebug() << "Enable dynamic mode:" << (preferences.dynamicModeEnabled ? "Y" : "N");
    qDebug() << "Non-repetition interval:"
             << preferences.trackRepetitionAvoidanceIntervalSeconds << "seconds";

    return 0;
}
