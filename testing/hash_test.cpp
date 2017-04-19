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

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QVector>

#include <functional>

/* TagLib includes */
#include <flacfile.h>
#include <id3v2framefactory.h>
#include <mpegfile.h>
#include <tbytevector.h>
#include <tfile.h>
#include <tbytevectorstream.h>

using namespace PMP;

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

QList<std::function<void (TagLib::File*)> > getModifiers(QString extension) {
    QList<std::function<void (TagLib::File*)> > modifiers;

    modifiers.append([](TagLib::File* file) { file->tag()->setTitle("Ooooooooo"); });
    modifiers.append([](TagLib::File* file) { file->tag()->setArtist("Aaaaaaaaaa"); });
    modifiers.append([](TagLib::File* file) { file->tag()->setAlbum("Eeeeeeeeee"); });
    modifiers.append([](TagLib::File* file) { file->tag()->setYear(2099); });
    modifiers.append([](TagLib::File* file) { file->tag()->setComment("No comment!"); });
    modifiers.append([](TagLib::File* file) {  });

    // TODO: format-specific file modifications

    return modifiers;
}

class FileTester {
public:
    FileTester(QString filename, QString expectedResult)
     : _out(stdout), _err(stderr), _error(false), _filename(filename),
       _expectedResult(expectedResult)
    {
        QFileInfo fileInfo(filename);
        _extension = fileInfo.suffix();

        if (!FileAnalyzer::isExtensionSupported(_extension, true)) {
            _err << "File extension not supported: " << _extension << endl;
            _error = true;
            return;
        }

        QFile file(filename);
        if (!file.open(QIODevice::ReadOnly)) {
            _err << "Could not open file: " << filename << endl;
            _error = true;
            return;
        }

        FileAnalyzer analyzer(fileInfo);
        analyzer.analyze();

        if (!analyzer.analysisDone()) {
            if (expectedResult == "invalid") return; /* OK */

            _err << "File analysis FAILED unexpectedly for " << filename << endl;
            _error = true;
            return;
        }

        _originalResult = getHashAsString(analyzer.hash());

        if (_originalResult != expectedResult) {
            _err << "Hash MISMATCH!" << endl
                 << "Filename: " << filename << endl
                 << "Expected: " << expectedResult << endl
                 << "Actual:   " << _originalResult << endl;
            _error = true;
            return;
        }

        QByteArray contents = file.readAll();
        _originalFileContents.setData(contents.data(), contents.length());
        _out << "Original data checksum: " << checksum(_originalFileContents) << endl;
    }

    bool error() const { return _error; }

    const TagLib::ByteVector& originalFileContents() const {
        return _originalFileContents;
    }

    const QString& extension() const { return _extension; }

    const QString& originalResult() const { return _originalResult; }

    bool testModifications(QList<std::function<void (TagLib::File*)> > modifiers) {
        QVector<TagLib::ByteVector> transformed;
        transformed.reserve(modifiers.size());

        /* Apply all modifications, and make sure they really are different modifications
           of the original. */
        Q_FOREACH(auto modifier, modifiers) {
            auto modifiedData = applyModification(modifier);
            _out << "Modified data checksum: " << checksum(modifiedData) << endl;

            if (modifiedData == originalFileContents()) {
                _err << "Problem: modification ineffective; test would be unreliable"
                     << endl;
                return false;
            }

            if (transformed.contains(modifiedData)) {
                _err << "Problem: modification not unique; test would be unreliable"
                     << endl;
                return false;
            }

            transformed.append(modifiedData);
        }

        Q_FOREACH(auto modifiedData, transformed) {
            FileAnalyzer analyzer(modifiedData, extension());
            analyzer.analyze();
            if (!analyzer.analysisDone()) {
                _err << "File analysis FAILED on modified data!" << endl;
                return false;
            }

            QString modifiedHash = getHashAsString(analyzer.hash());
            if (modifiedHash != _expectedResult) {
                _err << "Hash MISMATCH after modification!" << endl
                     << "Filename: " << _filename << endl
                     << "Expected: " << _expectedResult << endl
                     << "Original: " << originalResult() << endl
                     << "Modified: " << modifiedHash << endl;
                return false;
            }

            _out << "Modification resulted in correct hash." << endl;
        }

        return true;
    }

private:
    TagLib::ByteVector applyModification(
            std::function<TagLib::ByteVector (const TagLib::ByteVector&)> modifier)
    {
        return modifier(originalFileContents());
    }

    TagLib::ByteVector applyModification(std::function<void (TagLib::File*)> modifier) {
        /* create a stream from the original file contents (makes a copy) */
        TagLib::ByteVectorStream scratchStream(originalFileContents());

        TagLib::File* scratchFile = createFileObject(scratchStream, extension());

        if (scratchFile->readOnly() || !scratchFile->isOpen() || !scratchFile->isValid())
        {
            _err << "Problem when modifying tags: scratch file not modifyable" << endl;
            return TagLib::ByteVector();
        }

        modifier(scratchFile);

        if (!scratchFile->save()) {
            _err << "Problem when saving modified scratch file to scratch stream" << endl;
            return TagLib::ByteVector();
        }

        /* get the modified data from the scratch stream */
        return *scratchStream.data();
    }


    QTextStream _out;
    QTextStream _err;
    bool _error;
    QString _filename;
    QString _extension;
    QString _expectedResult;
    TagLib::ByteVector _originalFileContents;
    QString _originalResult;
};

void setArtistAndYear(TagLib::File* file) {
    file->tag()->setArtist("Aaaaaaaaaa");
    file->tag()->setYear(2099);
}

int main(int argc, char *argv[]) {

    QCoreApplication a(argc, argv);

    QCoreApplication::setApplicationName("Party Music Player - Hash test executable");
    QCoreApplication::setApplicationVersion("0.0.0.1");
    QCoreApplication::setOrganizationName("Party Music Player");
    QCoreApplication::setOrganizationDomain("hyperquantum.be");

    QTextStream out(stdout);
    QTextStream err(stderr);

    /* usage: PMP-HashTool <filename> <expected hash> */
    QStringList arguments = QCoreApplication::arguments();
    if (arguments.size() != 3 || arguments[1].isEmpty() || arguments[2].isEmpty())
    {
        err << "Exactly two non-empty arguments are required." << endl;
        return 2;
    }

    QString filename = arguments[1];
    QString expectedResult = arguments[2];

    FileTester tester(filename, expectedResult);
    if (tester.error()) return 1;

    bool result = tester.testModifications(getModifiers(tester.extension()));
    if (!result) return 1;

    out << "Success!" << endl;

    return 0;
}
