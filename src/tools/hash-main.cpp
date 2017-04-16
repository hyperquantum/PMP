/*
    Copyright (C) 2011-2017, Kevin Andre <hyperquantum@gmail.com>

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

#include "common/fileanalyzer.h"
#include "common/version.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

using namespace PMP;

int main(int argc, char *argv[]) {

    QCoreApplication a(argc, argv);

    QCoreApplication::setApplicationName("Party Music Player - Hash tool");
    QCoreApplication::setApplicationVersion(PMP_VERSION_DISPLAY);
    QCoreApplication::setOrganizationName(PMP_ORGANIZATION_NAME);
    QCoreApplication::setOrganizationDomain(PMP_ORGANIZATION_DOMAIN);

    QTextStream out(stdout);
    QTextStream err(stderr);

    if (QCoreApplication::arguments().size() < 2) {
        err << "No arguments given." << endl;
        return 0;
    }

    QString fileName = QCoreApplication::arguments()[1];
    QFileInfo fileInfo(fileName);

    if (!FileAnalyzer::isExtensionSupported(fileInfo.suffix(), true)) {
        err << "Files with extension \"" << fileInfo.suffix() << "\" are not supported."
            << endl;
        return 1;
    }

    bool isExperimentalFileFormat =
        FileAnalyzer::isExtensionSupported(fileInfo.suffix(), false);

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        err << "Could not open a file with that name." << "\n";
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

    if (isExperimentalFileFormat)
        out << "NOTICE: support for analyzing this file format is EXPERIMENTAL" << endl;

    FileAnalyzer analyzer(fileContents, fileInfo.suffix());
    analyzer.analyze();

    if (!analyzer.analysisDone()) {
        err << "Something went wrong when analyzing the file!" << endl;
        return 1;
    }

    FileHash finalHash = analyzer.hash();
    FileHash legacyHash = analyzer.legacyHash();

    out << "title:   " << analyzer.tagData().title() << endl;
    out << "artist:  " << analyzer.tagData().artist() << endl;
    out << "comment: " << analyzer.tagData().comment() << endl;
    out << "stripped file size: " << finalHash.length() << endl;
    out << "stripped MD5 Hash:  " << finalHash.MD5().toHex() << endl;
    out << "stripped SHA1 Hash: " << finalHash.SHA1().toHex() << endl;

    if (!legacyHash.empty()) {
        out << "legacy file size: " << legacyHash.length() << endl;
        out << "legacy MD5 Hash:  " << legacyHash.MD5().toHex() << endl;
        out << "legacy SHA1 Hash: " << legacyHash.SHA1().toHex() << endl;
    }

    return 0;
}
