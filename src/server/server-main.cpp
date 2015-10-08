/*
    Copyright (C) 2011-2015, Kevin Andre <hyperquantum@gmail.com>

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

#include "common/filedata.h"

#include "database.h"
#include "directoriesindexationtask.h"
#include "generator.h"
#include "history.h"
#include "player.h"
#include "queue.h"
#include "queueentry.h"
#include "resolver.h"
#include "server.h"
#include "serversettings.h"
#include "users.h"

#include <QCoreApplication>
#include <QStandardPaths>
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

int main(int argc, char *argv[]) {

    QCoreApplication app(argc, argv);

    QCoreApplication::setApplicationName("Party Music Player - Server");
    QCoreApplication::setApplicationVersion("0.0.0.1");
    QCoreApplication::setOrganizationName("Party Music Player");
    QCoreApplication::setOrganizationDomain("hyperquantum.be");

    QTextStream out(stdout);

    out << endl << "PMP --- Party Music Player" << endl << endl;

    /* seed random number generator */
    qsrand(QTime::currentTime().msec());

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

    Database::init(out);

//    FileData song =
//        FileData::create(
//            HashID(
//                4018772,
//                QByteArray::fromHex("b27e235c22f43a25a76a4b4916f7298359b7ed25"),
//                QByteArray::fromHex("b72e952ef61b3d69c649791b6ed583d4")
//            ),
//            "Gladys Knight", "License To Kill"
//        );

    Resolver resolver;
//    resolver.registerData(song);

    /* unique server instance ID (not to be confused with the unique ID of the database)*/
    QUuid serverInstanceIdentifier = QUuid::createUuid();
    out << "Server instance identifier: " << serverInstanceIdentifier.toString() << endl
        << endl;

    Users users;
    Player player(0, &resolver, defaultVolume);
    Queue& queue = player.queue();
    History history(&player);

    Generator generator(&queue, &resolver, &history);
    QObject::connect(
        &player, &Player::currentTrackChanged,
        &generator, &Generator::currentTrackChanged
    );
    QObject::connect(
        &player, &Player::userPlayingForChanged,
        &generator, &Generator::setUserPlayingFor
    );

    musicPaths.append("."); /* temporary, for backwards compatibility */
    resolver.setMusicPaths(musicPaths);

    out << endl
        << "Volume = " << player.volume() << endl;

    out << endl;

    Server server(0, serverInstanceIdentifier);
    if (!server.listen(&player, &generator, &users, QHostAddress::Any, 23432)) {
        out << "Could not start TCP listener: " << server.errorString() << endl;
        out << "Exiting." << endl;
        return 1;
    }

    out << "Now listening on port " << server.port() << endl
        << "Server password is " << server.serverPassword() << endl
        << endl;

    // exit when the server instance signals it
    QObject::connect(&server, SIGNAL(shuttingDown()), &app, SLOT(quit()));

    out << endl << "Server initialization complete. Starting indexation thread." << endl;

    /* start indexation of the media directories */
    DirectoriesIndexationTask* indexTask =
        new DirectoriesIndexationTask(&resolver, musicPaths);
    QThreadPool::globalInstance()->start(indexTask);

    return app.exec();
}
