/* 
   Copyright (C) 2011-2012, Kevin Andre <hyperquantum@gmail.com>
   
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

#include <QCryptographicHash>


int main(int argc, char *argv[]) {
	
	QCoreApplication a(argc, argv);
	
	QCoreApplication::setApplicationName("Party Music Player");
	QCoreApplication::setApplicationVersion("0.0.0.1");
	QCoreApplication::setOrganizationName("Party Music Player");
	QCoreApplication::setOrganizationDomain("hyperquantum.be");
	
	QTextStream out(stdout);
	
	QFile file("this_is_a_test.mp3");
	
	if (file.open(QIODevice::ReadOnly)) {
		QByteArray fileContents = file.readAll();
		
		QCryptographicHash md5_hasher(QCryptographicHash::Md5);
		md5_hasher.addData(fileContents);
		
		QCryptographicHash sha1_hasher(QCryptographicHash::Sha1);
		sha1_hasher.addData(fileContents);
		
		out << "File size: " << fileContents.length() << "\n";
		out << "MD5 Hash:  " << md5_hasher.result().toHex() << "\n";
		out << "SHA1 Hash: " << sha1_hasher.result().toHex() << "\n";
		
	}
	
	return 0;
}
