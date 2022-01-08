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
#include "server/serversettings.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QHostAddress>
#include <QHostInfo>
#include <QNetworkInterface>
#include <QSslSocket>
#include <QtDebug>
#include <QtGlobal>
#include <QThread>

using namespace PMP;

int main(int argc, char* argv[])
{
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

    for (const QHostAddress& address : QNetworkInterface::allAddresses()) {
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

    /*
    DatabaseConnectionSettings databaseConnectionSettings;
    databaseConnectionSettings.hostname = "localhost";
    databaseConnectionSettings.username = "root";
    databaseConnectionSettings.password = "xxxxxxxxxxx";

    if (!Database::init(out, databaseConnectionSettings))
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
    */

    auto d1 = QDateTime::currentDateTimeUtc();
    QThread::msleep(200);
    auto d2 = QDateTime::currentDateTimeUtc();
    QThread::msleep(200);
    auto d3 = QDateTime::currentDateTimeUtc();

    qDebug() << "d1-d2:" << d2.msecsTo(d1);
    qDebug() << "d2-d3:" << d3.msecsTo(d2);
    qDebug() << d1.toString(Qt::ISODateWithMs)
             << d2.toString(Qt::ISODateWithMs)
             << d3.toString(Qt::ISODateWithMs);

    auto msSinceEpoch = d1.toMSecsSinceEpoch();
    auto d1R = QDateTime::fromMSecsSinceEpoch(msSinceEpoch, Qt::UTC);

    qDebug() << d1R.toString(Qt::ISODateWithMs);

    return 0;
}
