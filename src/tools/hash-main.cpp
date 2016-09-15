/*
    Copyright (C) 2011-2016, Kevin Andre <hyperquantum@gmail.com>

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

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

using namespace PMP;

int main(int argc, char *argv[]) {

    QCoreApplication a(argc, argv);

    QCoreApplication::setApplicationName("Party Music Player - Hash tool");
    QCoreApplication::setApplicationVersion("0.0.0.1");
    QCoreApplication::setOrganizationName("Party Music Player");
    QCoreApplication::setOrganizationDomain("hyperquantum.be");

    QTextStream out(stdout);

    if (QCoreApplication::arguments().size() < 2) {
        out << "No arguments given." << endl;
        return 0;
    }

    QString fileName = QCoreApplication::arguments()[1];
    QFileInfo fileInfo(fileName);

    if (!FileData::supportsExtension(fileInfo.suffix())) {
        out << "Files with extension \"" << fileInfo.suffix() << "\" are not supported."
            << endl;
        return 1;
    }

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

    out << "File name: " << fileName << endl;
    out << "File size: " << fileContents.length() << endl;
    out << "MD5 Hash:  " << md5_hasher.result().toHex() << endl;
    out << "SHA1 Hash: " << sha1_hasher.result().toHex() << endl;

    FileData filedata = FileData::analyzeFile(fileContents, fileInfo.suffix());

    if (!filedata.isValid()) {
        out << "Something went wrong when analyzing the file!" << endl;
        return 1;
    }

    out << "title : " << filedata.tags().title() << endl;
    out << "artist: " << filedata.tags().artist() << endl;
    out << "stripped file size: " << filedata.hash().length() << endl;
    out << "stripped MD5 Hash:  " << filedata.hash().MD5().toHex() << endl;
    out << "stripped SHA1 Hash: " << filedata.hash().SHA1().toHex() << endl;

    return 0;
}
