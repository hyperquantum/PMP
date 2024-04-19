/*
    Copyright (C) 2011-2024, Kevin Andre <hyperquantum@gmail.com>

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

#include "common/concurrent.h"
#include "common/logging.h"
#include "common/util.h"
#include "common/version.h"

#include "collectionmonitor.h"
#include "database.h"
#include "delayedstart.h"
#include "generator.h"
#include "hashidregistrar.h"
#include "hashrelations.h"
#include "history.h"
#include "historystatistics.h"
#include "player.h"
#include "playerqueue.h"
#include "preloader.h"
#include "queueentry.h"
#include "resolver.h"
#include "scrobbling.h"
#include "selftest.h"
#include "serverhealthmonitor.h"
#include "serversettings.h"
#include "tcpserver.h"
#include "trackinfoproviderimpl.h"
#include "userhashstatscache.h"
#include "userhashstatscachefixer.h"
#include "users.h"

#include <QCoreApplication>
#include <QtDebug>
#include <QThreadPool>

using namespace PMP;
using namespace PMP::Server;

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
                             TcpServer const& server, Player const& player)
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

namespace
{
    bool initialIndexation = true;
}

static void setUpAndRunInitialIndexation(Resolver* resolver)
{
    QObject::connect(
        resolver, &Resolver::fullIndexationRunStatusChanged,
        resolver,
        [resolver]()
        {
            if (resolver->isFullIndexationRunning() || !initialIndexation)
                return;

            initialIndexation = false;
            QTextStream out(stdout);
            out << "Indexation finished." << Qt::endl
                << Qt::endl;
        }
    );

    QTextStream out(stdout);
    out << "Running initial full indexation..." << Qt::endl;
    resolver->startFullIndexation();
}

static void loadEquivalencesAndStartHashStatsCacheFixerAsync(
                                            HashRelations* hashRelations,
                                            UserHashStatsCacheFixer* hashStatsCacheFixer)
{
    auto equivalencesLoadingFuture =
        Concurrent::run<SuccessType, FailureType>(
            [hashRelations]() -> ResultOrError<SuccessType, FailureType>
            {
                auto db = Database::getDatabaseForCurrentThread();
                if (!db) return failure; /* database not available */

                auto equivalencesOrError = db->getEquivalences();
                if (equivalencesOrError.failed())
                    return failure;

                auto equivalences = equivalencesOrError.result();
                hashRelations->loadEquivalences(equivalences);
                qDebug() << "successfully loaded" << equivalences.size()
                         << "equivalences from the database";
                return success;
            }
        );

    equivalencesLoadingFuture.addListener(
        hashStatsCacheFixer,
        [hashStatsCacheFixer](ResultOrError<SuccessType, FailureType> result)
        {
            if (result.failed())
                qDebug() << "failed to load equivalences from the database";

            hashStatsCacheFixer->start();
        }
    );
}

static void startBackgroundTasks(HashRelations* hashRelations,
                                UserHashStatsCacheFixer* hashStatsCacheFixer)
{
    loadEquivalencesAndStartHashStatsCacheFixerAsync(hashRelations, hashStatsCacheFixer);
}

static int runServer(QCoreApplication& app, bool doIndexation);

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    QCoreApplication::setApplicationName("Party Music Player - Server");
    QCoreApplication::setApplicationVersion(PMP_VERSION_DISPLAY);
    QCoreApplication::setOrganizationName(PMP_ORGANIZATION_NAME);
    QCoreApplication::setOrganizationDomain(PMP_ORGANIZATION_DOMAIN);

    bool doIndexation = true;
    const QStringList args = QCoreApplication::arguments();
    for (auto& arg : args)
    {
        if (arg == "-no-index" || arg == "-no-indexation")
            doIndexation = false;
    }

    auto exitCode = runServer(app, doIndexation);

    qDebug() << "Exiting with code" << exitCode;

    return exitCode;
}

static int runServer(QCoreApplication& app, bool doIndexation)
{
    QTextStream out(stdout);

    const auto programNameVersionBuild =
        QString(VCS_REVISION_LONG).isEmpty()
            ? QString("Party Music Player %1")
                .arg(PMP_VERSION_DISPLAY)
            : QString("Party Music Player %1 build %2 (%3)")
                .arg(PMP_VERSION_DISPLAY,
                     VCS_REVISION_LONG,
                     VCS_BRANCH);

    out << Qt::endl
        << programNameVersionBuild << Qt::endl
        << Util::getCopyrightLine(true) << Qt::endl
        << "This is free software; see the source for copying conditions.  There is NO\n"
        << "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"
        << Qt::endl;

    /* set up logging */
    Logging::enableConsoleAndTextFileLogging(true);
    Logging::setFilenameTag("S"); /* S = Server */
    qDebug() << "PMP server started"; // initial log message
    Logging::cleanupOldLogfiles();
    /* TODO: do a log cleanup regularly, because a server is likely to be run for days,
     *       weeks, or months before being restarted. */

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

    SelfTest::runSelfTest(serverHealthMonitor);

    HashIdRegistrar hashIdRegistrar;
    HashRelations hashRelations;
    UserHashStatsCache userHashStatsCache;
    HistoryStatistics historyStatistics(nullptr, &hashRelations, &userHashStatsCache);
    Resolver resolver(&hashIdRegistrar, &hashRelations, &historyStatistics);

    Users users;
    Player player(nullptr, &resolver, serverSettings.defaultVolume());
    DelayedStart delayedStart(&player);
    PlayerQueue& queue = player.queue();
    History history(&player, &hashIdRegistrar, &historyStatistics);
    UserHashStatsCacheFixer hashStatsCacheFixer(&historyStatistics);

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

    TrackInfoProviderImpl trackInfoProvider(&resolver);

    Scrobbling scrobbling(nullptr, &trackInfoProvider);
    auto scrobblingController = scrobbling.getController();
    QObject::connect(
        &player, &Player::startedPlaying,
        scrobblingController,
        [scrobblingController](uint userPlayingFor, QDateTime startTime, QString title,
                               QString artist, QString album, QString albumArtist,
                               int trackDurationSeconds)
        {
            ScrobblingTrack track(title, artist, album, albumArtist);
            track.durationInSeconds = trackDurationSeconds;

            scrobblingController->updateNowPlaying(userPlayingFor, startTime, track);
        }
    );
    QObject::connect(
        &player, &Player::newHistoryEntry,
        scrobblingController,
        [scrobblingController](QSharedPointer<RecentHistoryEntry> entry)
        {
            scrobblingController->wakeUp(entry->user());
        }
    );
    scrobblingController->enableScrobbling();

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

    TcpServer server(nullptr, &serverSettings, serverInstanceIdentifier);
    bool listening =
        server.listen(&player, &generator, &history, &hashIdRegistrar, &hashRelations,
                      &users,
                      &collectionMonitor, &serverHealthMonitor, &scrobbling,
                      &delayedStart,
                      QHostAddress::Any, 23432);

    if (!listening)
    {
        reportStartupError(1, "Could not start TCP listener: " + server.errorString());
        return 1;
    }

    qDebug() << "Started listening to TCP port:" << server.port();

    // exit when the server instance signals it
    QObject::connect(&server, &TcpServer::shuttingDown, &app, &QCoreApplication::quit);

    printStartupSummary(out, serverSettings, server, player);

    startBackgroundTasks(&hashRelations/*, &historyPrefetcher*/, &hashStatsCacheFixer);

    out << "\n"
        << "Server initialization complete." << Qt::endl
        << Qt::endl;

    /* start indexation of the media directories */
    if (databaseInitializationSucceeded && doIndexation)
        setUpAndRunInitialIndexation(&resolver);

    auto exitCode = app.exec();

    out << "\n"
        << "Server exiting with code " << exitCode << "." << Qt::endl;

    return exitCode;
}
