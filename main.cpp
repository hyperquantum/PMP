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

#include "filedata.h"

#include <QtCore>

#include <QDirIterator>
#include <QFileInfo>
#include <QMediaPlayer>

using namespace PMP;

int main(int argc, char *argv[]) {

	QCoreApplication a(argc, argv);

	QCoreApplication::setApplicationName("Party Music Player");
	QCoreApplication::setApplicationVersion("0.0.0.1");
	QCoreApplication::setOrganizationName("Party Music Player");
	QCoreApplication::setOrganizationDomain("hyperquantum.be");

	QTextStream out(stdout);

	out << endl << "PMP --- Party Music Player" << endl << endl;

	QDirIterator it(".", QDirIterator::Subdirectories);
	uint fileCount = 0;
	QSet<HashID> uniqueFiles;
	QList<QString> pathsToPlay;
	while (it.hasNext()) {
		QFileInfo entry(it.next());
		if (!entry.isFile()) continue;
		if (entry.suffix().toLower() == "mp3") {
			QString path = entry.filePath();
			out << "  " << path << endl;

			FileData* data = FileData::analyzeFile(path);
			if (data == 0) {
				out << "     failed to analyze file!" << endl;
			}
			else {
				++fileCount;
				if(!uniqueFiles.contains(data->hash())) {
					uniqueFiles.insert(data->hash());
					pathsToPlay.append(path);
				}

				// FIXME: durations of 24 hours and longer will not work with this code
				QTime length = QTime(0, 0).addSecs(data->lengthInSeconds());

				out << "     " << length.toString() << endl
					<< "     " << data->artist() << endl
					<< "     " << data->title() << endl
					<< "     " << data->album() << endl
					<< "     " << data->comment() << endl
					<< "     " << data->hash().dumpToString() << endl;
				delete data;
			}
		}
	}

	out << endl
		<< fileCount << " files, " << uniqueFiles.size() << " unique hashes" << endl;

	if (pathsToPlay.empty()) {
        return 0; // nothing to play
	}

	QString file = pathsToPlay[0];
	out << "Will try to play " << file << endl;

    QEventLoop loop;

	QMediaPlayer player;
	player.setMedia(QUrl::fromLocalFile(file));

    player.setVolume(75);
    out << "volume=" << player.volume() << endl;

    player.play();

	return a.exec();
}
