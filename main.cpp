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


#include <QtCore>

#include <QDirIterator>
#include <QFileInfo>

int main(int argc, char *argv[]) {
	
	QCoreApplication a(argc, argv);
	
	QCoreApplication::setApplicationName("Party Music Player");
	QCoreApplication::setApplicationVersion("0.0.0.1");
	QCoreApplication::setOrganizationName("Party Music Player");
	QCoreApplication::setOrganizationDomain("hyperquantum.be");
	
	QTextStream out(stdout);
	
	out << endl << "PMP --- Party Music Player" << endl << endl;
	
	QDirIterator it(".", QDirIterator::Subdirectories);
	while (it.hasNext()) {
		QFileInfo entry(it.next());
		if (!entry.isFile()) continue;
		if (entry.suffix().toLower() == "mp3") {
			out << "  " << entry.filePath() << endl;
		}
	}
	
	return 0;
}
