/*
    Copyright (C) 2011-2021, Kevin Andre <hyperquantum@gmail.com>

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

#include "collectionmonitor.h"
#include "database.h"
#include "generator.h"
#include "history.h"
#include "player.h"
#include "playerqueue.h"
#include "preloader.h"
#include "queueentry.h"
#include "resolver.h"
#include "server.h"
#include "serverhealthmonitor.h"
#include "serversettings.h"
#include "users.h"

#include <QCoreApplication>
#include <QtDebug>
#include <QThreadPool>

using namespace PMP;

namespace
{
    void reportStartupError(int exitCode, QString message)
    {
        QTextStream err(stderr);
        err << message << '\n'
            << "Exiting." << Qt::endl;

        // write to log file
        qDebug() << "Startup error:" << message;
        qDebug() << "Will exit with code" << exitCode;
    }

    void printStartupSummary(QTextStream& out, ServerSettings const& serverSettings,
                             Server const& server, Player const& player)
    {
        out << "Server instance identifier: " << server.uuid().toString() << "\n";
        out << "Server caption: " << server.caption() << "\n";

        out << Qt::endl;

        out << "Music paths to scan:\n";
        auto const musicPaths = serverSettings.musicPaths();
        for (QString const& path : musicPaths)
        {
            out << "  " << path << "\n";
        }

        out << Qt::endl;

        out << "Volume: " << player.volume() << "\n";

        out << Qt::endl;

        out << "Now listening on port " << server.port() << "\n"
            << "Server password is " << server.serverPassword() << "\n";

        out << Qt::endl;
    }
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    QCoreApplication::setApplicationName("Party Music Player - Server");
    QCoreApplication::setApplicationVersion(PMP_VERSION_DISPLAY);
    QCoreApplication::setOrganizationName(PMP_ORGANIZATION_NAME);
    QCoreApplication::setOrganizationDomain(PMP_ORGANIZATION_DOMAIN);

    QTextStream out(stdout);

    bool doIndexation = true;
    QStringList args = QCoreApplication::arguments();
    Q_FOREACH(QString arg, args)
    {
        if (arg == "-no-index" || arg == "-no-indexation")
            doIndexation = false;
    }

    out << Qt::endl
        << "Party Music Player - version " PMP_VERSION_DISPLAY << Qt::endl
        << Util::getCopyrightLine(true) << Qt::endl
        << Qt::endl;

    /* set up logging */
    Logging::enableConsoleAndTextFileLogging(true);
    Logging::setFilenameTag("S"); /* S = Server */
    qDebug() << "PMP server started"; // initial log message
    Logging::cleanupOldLogfiles();
    /* TODO: do a log cleanup regularly, because a server is likely to be run for days,
     *       weeks, or months before being restarted. */

    /* seed random number generator */
    // FIXME : stop using qrand(), and then we can remove this call to qsrand()
    qsrand(QTime::currentTime().msec());

    ServerHealthMonitor serverHealthMonitor;

    ServerSettings serverSettings;
    serverSettings.load();

    Preloader::cleanupOldFiles();

    //for (const QString &path : app.libraryPaths())
    //    out << " LIB PATH : " << path << endl;

    /* Keep the threads of the thread pool alive, so we don't have to generate a new
     * database connection almost everytime a new task is started. */
    QThreadPool::globalInstance()->setExpiryTimeout(-1);

    bool databaseInitializationSucceeded = Database::init(out, serverSettings);
    if (!databaseInitializationSucceeded)
    {
        serverHealthMonitor.setDatabaseUnavailable();
    }

    Resolver resolver;

    Users users;
    Player player(nullptr, &resolver, serverSettings.defaultVolume());
    PlayerQueue& queue = player.queue();
    History history(&player);

    CollectionMonitor collectionMonitor;
    QObject::connect(
        &resolver, &Resolver::hashBecameAvailable,
        &collectionMonitor, &CollectionMonitor::hashBecameAvailable
    );
    QObject::connect(
        &resolver, &Resolver::hashBecameUnavailable,
        &collectionMonitor, &CollectionMonitor::hashBecameUnavailable
    );
    QObject::connect(
        &resolver, &Resolver::hashTagInfoChanged,
        &collectionMonitor, &CollectionMonitor::hashTagInfoChanged
    );

    Generator generator(&queue, &resolver, &history);
    QObject::connect(
        &player, &Player::currentTrackChanged,
        &generator, &Generator::currentTrackChanged
    );
    QObject::connect(
        &player, &Player::userPlayingForChanged,
        &generator, &Generator::setUserGeneratingFor
    );

    resolver.setMusicPaths(serverSettings.musicPaths());
    QObject::connect(
        &serverSettings, &ServerSettings::musicPathsChanged,
        &resolver,
        [&serverSettings, &resolver]()
        {
            resolver.setMusicPaths(serverSettings.musicPaths());
        }
    );

    /* unique server instance ID (not to be confused with the unique ID of the database)*/
    QUuid serverInstanceIdentifier = QUuid::createUuid();

    Server server(nullptr, &serverSettings, serverInstanceIdentifier);
    bool listening =
        server.listen(&player, &generator, &history, &users, &collectionMonitor,
                      &serverHealthMonitor, QHostAddress::Any, 23432);

    if (!listening)
    {
        reportStartupError(1, "Could not start TCP listener: " + server.errorString());
        return 1;
    }

    qDebug() << "Started listening to TCP port:" << server.port();

    // exit when the server instance signals it
    QObject::connect(&server, &Server::shuttingDown, &app, &QCoreApplication::quit);

    printStartupSummary(out, serverSettings, server, player);

    out << "\n"
        << "Server initialization complete." << Qt::endl
        << Qt::endl;

    /* start indexation of the media directories */
    if (databaseInitializationSucceeded && doIndexation)
    {
        bool initialIndexation = true;

        QObject::connect(
            &resolver, &Resolver::fullIndexationRunStatusChanged,
            &resolver,
            [&out, &initialIndexation](bool running)
            {
                if (running || !initialIndexation)
                    return;

                initialIndexation = false;
                out << "Indexation finished." << Qt::endl
                    << Qt::endl;
            }
        );

        out << "Running initial full indexation..." << Qt::endl;
        resolver.startFullIndexation();
    }

    auto exitCode = app.exec();

    qDebug() << "Exiting with code" << exitCode;

    return exitCode;
}
