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

#include "fileanalysistask.h"
#include "generator.h"
#include "history.h"
#include "player.h"
#include "queue.h"
#include "resolver.h"
#include "server.h"

#include <QCoreApplication>
#include <QDirIterator>
#include <QFileInfo>
#include <QThreadPool>

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

    Player player(0, &resolver);
    Queue& queue = player.queue();
    History history(&player);

    Generator generator(&queue, &resolver, &history);
    QObject::connect(&player, SIGNAL(currentTrackChanged(QueueEntry const*)), &generator, SLOT(currentTrackChanged(QueueEntry const*)));

    QDirIterator it(".", QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QFileInfo entry(it.next());
        if (!entry.isFile()) continue;

        if (!FileData::supportsExtension(entry.suffix())) continue;

        QString path = entry.absoluteFilePath();

        qDebug() << "starting background analysis of" << path;

        FileAnalysisTask* task = new FileAnalysisTask(path);
        resolver.connect(task, SIGNAL(finished(QString, FileData*)), &resolver, SLOT(analysedFile(QString, FileData*)));
        QThreadPool::globalInstance()->start(task);
    }

    generator.enable();

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
