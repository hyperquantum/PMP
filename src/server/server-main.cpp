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
#include "scrobbling.h"
#include "selftest.h"
#include "server.h"
#include "serverhealthmonitor.h"
#include "serversettings.h"
#include "users.h"

#include <QCoreApplication>
#include <QStandardPaths>
#include <QtDebug>
#include <QThreadPool>

using namespace PMP;

QStringList generateDefaultScanPaths() {
    QStringList paths;
    paths.reserve(3);

    paths.append(QStandardPaths::standardLocations(QStandardPaths::MusicLocation));

    QStringList documentsPaths =
        QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation);
    paths.append(documentsPaths);

    /* Qt <5.3 returns the documents location as downloads location */
    QStringList downloadsPaths =
        QStandardPaths::standardLocations(QStandardPaths::DownloadLocation);
    Q_FOREACH(QString path, downloadsPaths) {
        if (!documentsPaths.contains(path)) paths.append(path);
    }

    return paths;
}

void reportStartupError(int exitCode, QString message)
{
    QTextStream err(stderr);
    err << message << endl
        << "Exiting." << endl;

    // write to log file
    qDebug() << "Startup error:" << message;
    qDebug() << "Will exit with code" << exitCode;
}

int main(int argc, char *argv[]) {

    QCoreApplication app(argc, argv);

    QCoreApplication::setApplicationName("Party Music Player - Server");
    QCoreApplication::setApplicationVersion(PMP_VERSION_DISPLAY);
    QCoreApplication::setOrganizationName(PMP_ORGANIZATION_NAME);
    QCoreApplication::setOrganizationDomain(PMP_ORGANIZATION_DOMAIN);

    QTextStream out(stdout);

    bool doIndexation = true;
    QStringList args = QCoreApplication::arguments();
    Q_FOREACH(QString arg, args) {
        if (arg == "-no-index" || arg == "-no-indexation")
            doIndexation = false;
    }

    out << endl
        << "Party Music Player - version " PMP_VERSION_DISPLAY << endl
        << Util::getCopyrightLine(true) << endl
        << endl;

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

    /* clean up leftover preloader cache files */
    Preloader::cleanupOldFiles();

    //foreach (const QString &path, app.libraryPaths())
    //    out << " LIB PATH : " << path << endl;

    /* Keep the threads of the thread pool alive, so we don't have to generate a new
     * database connection almost everytime a new task is started. */
    QThreadPool::globalInstance()->setExpiryTimeout(-1);

    QStringList musicPaths;
    int defaultVolume = -1;
    {
        ServerSettings serversettings;
        QSettings& settings = serversettings.getSettings();

        QVariant defaultVolumeSetting = settings.value("player/default_volume");
        if (defaultVolumeSetting.isValid()
            && defaultVolumeSetting.toString() != "")
        {
            bool ok;
            defaultVolume = defaultVolumeSetting.toString().toInt(&ok);
            if (!ok || defaultVolume < 0 || defaultVolume > 100) {
                out << "Invalid default volume setting found. Ignoring." << endl << endl;
                defaultVolume = -1;
            }
        }
        if (defaultVolume < 0) {
            settings.setValue("player/default_volume", "");
        }

        QVariant musicPathsSetting = settings.value("media/scan_directories");
        if (!musicPathsSetting.isValid() || musicPathsSetting.toStringList().empty()) {
            out << "No music paths set.  Setting default paths." << endl << endl;
            musicPaths = generateDefaultScanPaths();
            settings.setValue("media/scan_directories", musicPaths);
        }
        else {
            musicPaths = musicPathsSetting.toStringList();
        }

        out << "Music paths to scan:" << endl;
        Q_FOREACH(QString path, musicPaths) {
            out << "  " << path << endl;
        }
        out << endl;
    }

    bool databaseInitializationSucceeded = Database::init(out);
    if (!databaseInitializationSucceeded) {
        serverHealthMonitor.setDatabaseUnavailable();
    }

    SelfTest::runSelfTest(serverHealthMonitor);

    Resolver resolver;

    /* unique server instance ID (not to be confused with the unique ID of the database)*/
    QUuid serverInstanceIdentifier = QUuid::createUuid();
    out << "Server instance identifier: " << serverInstanceIdentifier.toString() << endl
        << endl;

    Users users;
    Player player(nullptr, &resolver, defaultVolume);
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

    Scrobbling scrobbling(nullptr, &resolver);
    auto scrobblingController = scrobbling.getController();
    QObject::connect(
        &player, &Player::startedPlaying,
        scrobblingController, &ScrobblingController::updateNowPlaying
    );
    QObject::connect(
        &player, &Player::newHistoryEntry,
        scrobblingController,
        [scrobblingController](QSharedPointer<PlayerHistoryEntry> entry)
        {
            scrobblingController->wakeUp(entry->user());
        }
    );
    QObject::connect(
        &resolver, &Resolver::fullIndexationRunStatusChanged,
        scrobblingController,
        [scrobblingController](bool running)
        {
            if (!running) /* if indexation completed... */
                emit scrobblingController->enableScrobbling();
        }
    );

    resolver.setMusicPaths(musicPaths);

    out << endl
        << "Volume = " << player.volume() << endl;

    out << endl;

    Server server(nullptr, serverInstanceIdentifier);
    bool listening =
        server.listen(&player, &generator, &history, &users, &collectionMonitor,
                      &serverHealthMonitor, &scrobbling, QHostAddress::Any, 23432);

    if (!listening)
    {
        reportStartupError(1, "Could not start TCP listener: " + server.errorString());
        return 1;
    }

    qDebug() << "Started listening to TCP port:" << server.port();
    out << "Now listening on port " << server.port() << endl
        << "Server password is " << server.serverPassword() << endl
        << endl;

    // exit when the server instance signals it
    QObject::connect(&server, &Server::shuttingDown, &app, &QCoreApplication::quit);

    out << endl << "Server initialization complete." << endl;

    /* start indexation of the media directories */
    if (databaseInitializationSucceeded && doIndexation)
        resolver.startFullIndexation();

    auto exitCode = app.exec();
    
    qDebug() << "Exiting with code" << exitCode;
    out << endl << "Server exiting with code " << exitCode << "." << endl;
    
    return exitCode;
}
