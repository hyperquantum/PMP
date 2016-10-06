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

#include "common/fileanalyzer.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

/* TagLib includes */
#include <flacfile.h>
#include <id3v2framefactory.h>
#include <mpegfile.h>
#include <tbytevector.h>
#include <tfile.h>
#include <tbytevectorstream.h>

using namespace PMP;

TagLib::File* createFileObject(TagLib::ByteVectorStream& fileStream, QString extension) {
    QString lowercaseExtension = extension.toLower();

    if (lowercaseExtension == "mp3") {
        return new TagLib::MPEG::File(&fileStream,
                                      TagLib::ID3v2::FrameFactory::instance());
    }
    else if (lowercaseExtension == "flac") {
        return new TagLib::FLAC::File(&fileStream,
                                      TagLib::ID3v2::FrameFactory::instance());
    }

    /* filetype not supported */
    return nullptr;
}

QString checksum(TagLib::ByteVector const& data) {
    QCryptographicHash sha1Hasher(QCryptographicHash::Sha1);
    sha1Hasher.addData(data.data(), data.size());

    return sha1Hasher.result().toHex();
}

QString getHashAsString(const FileHash& hash) {
    if (hash.empty()) return "empty";

    return QString::number(hash.length())
            + "-" + hash.MD5().toHex() + "-" + hash.SHA1().toHex();
}

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
    QString extension = fileInfo.suffix();

    if (!FileAnalyzer::isExtensionSupported(extension)) {
        err << "File extension not supported: " << extension << endl;
        return 2;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        err << "Could not open file: " << fileName << endl;
        return 2;
    }

    FileAnalyzer analyzer(fileInfo);
    analyzer.analyze();

    if (!analyzer.analysisDone()) {
        if (expectedResult == "invalid") return 0; /* OK */

        err << "File analysis FAILED unexpectedly." << endl;
        return 1;
    }

    QString hashAsString = getHashAsString(analyzer.hash());

    if (hashAsString != expectedResult) {
        err << "Hash MISMATCH!" << endl
            << "Filename: " << fileName << endl
            << "Expected: " << expectedResult << endl
            << "Actual:   " << hashAsString << endl;
        return 1;
    }

    QByteArray contents = file.readAll();
    TagLib::ByteVector contentsOriginal(contents.data(), contents.length());
    QString originalChecksum = checksum(contentsOriginal);

    TagLib::ByteVector scratch(contentsOriginal);
    TagLib::ByteVectorStream scratchStream(scratch);
    TagLib::File* scratchFile = createFileObject(scratchStream, extension);
    if (scratchFile->readOnly() || !scratchFile->isOpen() || !scratchFile->isValid()) {
        err << "Scratch memory not valid or readonly!" << endl
            << "Filename: " << fileName << endl;
        return 1;
    }

    if (scratchFile->tag()->year() != 2016) {
        err << "Tags not read correctly!" << endl
            << "Filename: " << fileName << endl;
        return 1;
    }

    scratchFile->tag()->setArtist("Aaaaaaaaaa");
    scratchFile->tag()->setYear(2099);
    if (!scratchFile->save()) {
        err << "Call to save() failed!" << endl
            << "Filename: " << fileName << endl;
        return 1;
    }

    /* Get the bytevector from the stream again, because the stream keeps its changes
     * local. */
    scratch = *scratchStream.data();

//    QString originalChecksumAgain = checksum(contentsOriginal);
//    if (originalChecksum != originalChecksumAgain) {
//        err << "Modification in scratch memory has affected original copy!" << endl
//            << "Filename: " << fileName << endl
//            << "Checksum at start: " << originalChecksum << endl
//            << "Checksum now:      " << originalChecksumAgain << endl;
//        return 1;
//    }

//    if (contentsOriginal.data() == scratch.data()) {
//        err << "Scratch memory still points to same memory as original data!" << endl
//            << "Filename: " << fileName << endl;
//        return 1;
//    }

    if (scratch == contentsOriginal) {
        err << "Changing artist in scratch buffer did not have any effect!" << endl
            << "Filename: " << fileName << endl
            << "Checksum of original: " << originalChecksum << endl
            << "Checksum of scratch:  " << checksum(scratch);
        return 1;
    }

    //FileData newData = FileData::analyzeFile(scratch, extension);
    FileAnalyzer analyzer2(scratch, extension);
    analyzer2.analyze();
    if (!analyzer2.analysisDone()) {
        err << "File analysis FAILED during scratch operations!" << endl;
        return 1;
    }

    QString scratchHash = getHashAsString(analyzer2.hash());
    if (scratchHash != expectedResult) {
        err << "Hash MISMATCH after scratch operation!" << endl
            << "Filename: " << fileName << endl
            << "Expected: " << expectedResult << endl
            << "Original: " << hashAsString << endl
            << "Modified: " << scratchHash << endl;
        return 1;
    }

    return 0;
}
