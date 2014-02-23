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

#include <QCryptographicHash>

#include <taglib/fileref.h>
#include <taglib/id3v2framefactory.h>
#include <taglib/mpegfile.h>
#include <taglib/tag.h>
#include <taglib/tbytevector.h>
#include <taglib/tbytevectorstream.h>


int main(int argc, char *argv[]) {
	
	QCoreApplication a(argc, argv);
	
	QCoreApplication::setApplicationName("Party Music Player - Hash tool");
	QCoreApplication::setApplicationVersion("0.0.0.1");
	QCoreApplication::setOrganizationName("Party Music Player");
	QCoreApplication::setOrganizationDomain("hyperquantum.be");
	
	QTextStream out(stdout);
	
	if (QCoreApplication::arguments().size() <= 1) {
		out << "No arguments given." <<  "\n";
		return 0;
	}
	
	QString fileName = QCoreApplication::arguments()[1];
	QFile file(fileName);
	
	if (!file.open(QIODevice::ReadOnly)) {
		out << "Could not open a file with that name." << "\n";
		return 1;
	}
	
	QByteArray fileContents = file.readAll();
	
	QCryptographicHash md5_hasher(QCryptographicHash::Md5);
	md5_hasher.addData(fileContents);
	
	QCryptographicHash sha1_hasher(QCryptographicHash::Sha1);
	sha1_hasher.addData(fileContents);
	
	out << "File name: " << fileName << "\n";
	out << "File size: " << fileContents.length() << "\n";
	out << "MD5 Hash:  " << md5_hasher.result().toHex() << "\n";
	out << "SHA1 Hash: " << sha1_hasher.result().toHex() << "\n";
	
	//TagLib::FileRef tagFile(fileName.toUtf8());
	
	TagLib::ByteVector fileContentsScratch(fileContents.data(), fileContents.length());
	TagLib::ByteVectorStream fileScratchStream(fileContentsScratch);
	
	TagLib::MPEG::File tagFile(&fileScratchStream, TagLib::ID3v2::FrameFactory::instance());
	
	TagLib::Tag* tag = tagFile.tag();
	if (tag == 0) {
		out << "no tags found" << "\n";
	}
	else {
		out << "artist: " << TStringToQString(tag->artist()) << "\n";
		out << "title: " << TStringToQString(tag->title()) << "\n";
	}
	
	tagFile.strip(); // strip all tag headers
	
	TagLib::ByteVector* stripped_data = fileScratchStream.data();
	
	QCryptographicHash md5_hasher_stripped(QCryptographicHash::Md5);
	md5_hasher_stripped.addData(stripped_data->data(), stripped_data->size());
	
	QCryptographicHash sha1_hasher_stripped(QCryptographicHash::Sha1);
	sha1_hasher_stripped.addData(stripped_data->data(), stripped_data->size());
	
	out << "stripped file size: " << stripped_data->size() << "\n";
	out << "stripped MD5 Hash:  " << md5_hasher_stripped.result().toHex() << "\n";
	out << "stripped SHA1 Hash: " << sha1_hasher_stripped.result().toHex() << "\n";
	
	return 0;
}
