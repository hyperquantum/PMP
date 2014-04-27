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

#include "player.h"
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

    Resolver resolver;

    QDirIterator it(".", QDirIterator::Subdirectories);
    uint fileCount = 0;
    QSet<HashID> uniqueFiles;
    while (it.hasNext()) {
        QFileInfo entry(it.next());
        if (!entry.isFile()) continue;

        if (!FileData::supportsExtension(entry.suffix())) continue;

        QString path = entry.absoluteFilePath();
        out << "  " << path << endl;

        FileData const* data = FileData::analyzeFile(path);
        if (data == 0) {
            out << "     failed to analyze file!" << endl;
            continue;
        }

        resolver.registerFile(data, path);

        if (data->lengthInSeconds() <= 10) {
            out << "     skipping file because length (" << data->lengthInSeconds() << ") unknown or not larger than 10 seconds" << endl;
            continue;
        }

        ++fileCount;
        uniqueFiles.insert(data->hash());

        // FIXME: durations of 24 hours and longer will not work with this code
        QTime length = QTime(0, 0).addSecs(data->lengthInSeconds());

        out << "     " << length.toString() << endl
            << "     " << data->artist() << endl
            << "     " << data->title() << endl
            << "     " << data->album() << endl
            << "     " << data->comment() << endl
            << "     " << data->hash().dumpToString() << endl;
    }

    out << endl
        << fileCount << " files, " << uniqueFiles.size() << " unique hashes" << endl;

    if (uniqueFiles.empty()) {
        return 0; // nothing to play
    }

    Player player(0, &resolver);

    out << endl
        << "Adding to queue:" << endl;

    QSet<HashID> queuedHashes;
    for (QSet<HashID>::const_iterator it = uniqueFiles.begin(); queuedHashes.count() < 10 && it != uniqueFiles.end(); ++it) {
        out << " - " << it->dumpToString() << endl;
        queuedHashes.insert(*it);
        player.queue(*it);
    }

    out << endl
        << "Volume = " << player.volume() << endl;

    out << endl;

    Server server;
    if (!server.listen(&player, QHostAddress::Any, 23432)) {
        out << "Could not start TCP listener: " << server.errorString() << endl;
        out << "Exiting." << endl;
        return 1;
    }

    out << "Now listening on port " << server.port() << endl;

    // exit when the server instance signals it
    QObject::connect(&server, SIGNAL(shuttingDown()), &app, SLOT(quit()));

    return app.exec();
}
