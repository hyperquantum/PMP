/*
    Copyright (C) 2011-2014, Kevin Andre <hyperquantum@gmail.com>

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

#include "generator.h"
#include "history.h"
#include "player.h"
#include "queue.h"
#include "resolver.h"
#include "server.h"

#include <QCoreApplication>
#include <QDirIterator>
#include <QFileInfo>
#include <QTime>

using namespace PMP;

int main(int argc, char *argv[]) {

    QCoreApplication app(argc, argv);

    QCoreApplication::setApplicationName("Party Music Player - Server");
    QCoreApplication::setApplicationVersion("0.0.0.1");
    QCoreApplication::setOrganizationName("Party Music Player");
    QCoreApplication::setOrganizationDomain("hyperquantum.be");

    QTextStream out(stdout);

    out << endl << "PMP --- Party Music Player" << endl << endl;

    //foreach (const QString &path, app.libraryPaths())
    //    out << " LIB PATH : " << path << endl;

    FileData song =
        FileData::create(
            HashID(4018772, QByteArray::fromHex("b27e235c22f43a25a76a4b4916f7298359b7ed25"), QByteArray::fromHex("b72e952ef61b3d69c649791b6ed583d4")),
            "Gladys Knight", "License To Kill"
        );

    // fake hash for testing a hash with no known paths
    FileData song2 =
        FileData::create(
            HashID(4104358, QByteArray::fromHex("abc1235c22f43a25a5874b496747298359b7ed25"), QByteArray::fromHex("1239872ef61b3d69c649791b6ed583d4")),
            "Gunther", "Ding Dong Song"
        );

    Resolver resolver;
    resolver.registerData(song);
    resolver.registerData(song2);

    QDirIterator it(".", QDirIterator::Subdirectories);
    uint fileCount = 0;
    QSet<HashID> uniqueFiles;
    while (it.hasNext()) {
        QFileInfo entry(it.next());
        if (!entry.isFile()) continue;

        if (!FileData::supportsExtension(entry.suffix())) continue;

        QString path = entry.absoluteFilePath();
        out << "  " << path << endl;

        FileData data = FileData::analyzeFile(path);
        if (!data.isValid()) {
            out << "     failed to analyze file!" << endl;
            continue;
        }

        resolver.registerFile(data, path);

        if (data.audio().trackLength() <= 10) {
            out << "     skipping file because length (" << data.audio().trackLength() << ") unknown or not larger than 10 seconds" << endl;
            continue;
        }

        ++fileCount;
        uniqueFiles.insert(data.hash());

        // FIXME: durations of 24 hours and longer will not work with this code
        QTime length = QTime(0, 0).addSecs(data.audio().trackLength());

        out << "     " << length.toString() << endl
            << "     " << data.tags().artist() << endl
            << "     " << data.tags().title() << endl
            << "     " << data.tags().album() << endl
            << "     " << data.tags().comment() << endl
            << "     " << data.hash().dumpToString() << endl;
    }

    out << endl
        << fileCount << " files, " << uniqueFiles.size() << " unique hashes" << endl;

    if (uniqueFiles.empty()) {
        return 0; // nothing to play
    }

    Player player(0, &resolver);

    History history(&player);

    Queue& queue = player.queue();

    Generator generator(&queue, &resolver, &history);
    QObject::connect(&player, SIGNAL(currentTrackChanged(QueueEntry const*)), &generator, SLOT(currentTrackChanged(QueueEntry const*)));

    out << endl
        << "Adding to queue:" << endl;

    QSet<HashID> queuedHashes;
    for (QSet<HashID>::const_iterator it = uniqueFiles.begin(); queuedHashes.count() < 10 && it != uniqueFiles.end(); ++it) {
        out << " - " << it->dumpToString() << endl;
        queuedHashes.insert(*it);
        queue.enqueue(*it);
    }

    out << endl
        << "Volume = " << player.volume() << endl;

    out << endl;

    Server server;
    if (!server.listen(&player, &generator, QHostAddress::Any, 23432)) {
        out << "Could not start TCP listener: " << server.errorString() << endl;
        out << "Exiting." << endl;
        return 1;
    }

    out << "Now listening on port " << server.port() << endl
        << endl;

    // exit when the server instance signals it
    QObject::connect(&server, SIGNAL(shuttingDown()), &app, SLOT(quit()));

    return app.exec();
}
