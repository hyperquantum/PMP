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
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

using namespace PMP;

int main(int argc, char *argv[]) {

    QCoreApplication a(argc, argv);

    QCoreApplication::setApplicationName("Party Music Player - Hash test executable");
    QCoreApplication::setApplicationVersion("0.0.0.1");
    QCoreApplication::setOrganizationName("Party Music Player");
    QCoreApplication::setOrganizationDomain("hyperquantum.be");

    QTextStream out(stdout);
    QTextStream err(stderr);

    if (QCoreApplication::arguments().size() != 3) { /* exe-name + 2 args */
        err << "Exactly two arguments are required." << endl;
        return 2;
    }

    QString fileName = QCoreApplication::arguments()[1];
    QString expectedResult = QCoreApplication::arguments()[2];

    QFileInfo fileInfo(fileName);

    if (!FileData::supportsExtension(fileInfo.suffix())) {
        err << "File extension not supported: " << fileInfo.suffix() << endl;
        return 2;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        err << "Could not open file: " << fileName << endl;
        return 2;
    }

    FileData filedata = FileData::analyzeFile(fileInfo);

    if (!filedata.isValid() || filedata.hash().empty()) {
        if (expectedResult == "invalid") return 0; /* OK */

        err << "File analysis FAILED unexpectedly." << endl;
        return 1;
    }

    const FileHash& hash = filedata.hash();
    QString hashAsString =
        QString::number(hash.length())
            + "-" + hash.MD5().toHex()
            + "-" + hash.SHA1().toHex();

    if (hashAsString == expectedResult) return 0; /* OK, great! */

    err << "Hash MISMATCH!" << endl
        << "Filename: " << fileName << endl
        << "Expected: " << expectedResult << endl
        << "Actual:   " << hashAsString << endl;

    return 1;
}
