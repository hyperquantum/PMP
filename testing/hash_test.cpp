/*
    Copyright (C) 2011-2021, Kevin Andre <hyperquantum@gmail.com>

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
#include <QDataStream>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QTextStream>
#include <QVector>

#include <functional>

/* TagLib includes */
#include <taglib/apetag.h>
#include <taglib/flacfile.h>
#include <taglib/id3v1tag.h>
#include <taglib/id3v2framefactory.h>
#include <taglib/id3v2tag.h>
#include <taglib/mpegfile.h>
#include <taglib/tbytevector.h>
#include <taglib/tfile.h>
#include <taglib/tbytevectorstream.h>
#include <taglib/xiphcomment.h>

using namespace PMP;

QString checksum(TagLib::ByteVector const& data) {
    QCryptographicHash sha1Hasher(QCryptographicHash::Sha1);
    sha1Hasher.addData(data.data(), data.size());

    return sha1Hasher.result().toHex();
}

QString getHashAsString(const FileHash& hash) {
    if (hash.isNull()) return "empty";

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

QList<std::function<void (TagLib::ID3v1::Tag*)> > getId3v1Modifiers() {
    QList<std::function<void (TagLib::ID3v1::Tag*)> > modifiers;

    modifiers.append([](TagLib::ID3v1::Tag* id3v1) { id3v1->setTitle("T7777tttt77"); });
    modifiers.append([](TagLib::ID3v1::Tag* id3v1) { id3v1->setArtist("L1111llll11"); });
    modifiers.append([](TagLib::ID3v1::Tag* id3v1) { id3v1->setAlbum("O0000oooo00"); });
    modifiers.append([](TagLib::ID3v1::Tag* id3v1) { id3v1->setYear(2097); });
    modifiers.append([](TagLib::ID3v1::Tag* id3v1) { id3v1->setComment("1 Hello ID3"); });

    return modifiers;
}

QList<std::function<void (TagLib::ID3v2::Tag*)> > getId3v2Modifiers() {
    QList<std::function<void (TagLib::ID3v2::Tag*)> > modifiers;

    modifiers.append([](TagLib::ID3v2::Tag* id3v2) { id3v2->setTitle("Qqqqq1234qq"); });
    modifiers.append([](TagLib::ID3v2::Tag* id3v2) { id3v2->setArtist("Ddd7788ddd"); });
    modifiers.append([](TagLib::ID3v2::Tag* id3v2) { id3v2->setAlbum("Rrrrr5005rrr"); });
    modifiers.append([](TagLib::ID3v2::Tag* id3v2) { id3v2->setYear(2098); });
    modifiers.append([](TagLib::ID3v2::Tag* id3v2) { id3v2->setComment("2 Hello ID3"); });

    return modifiers;
}

QList<std::function<void (TagLib::APE::Tag*)> > getApeModifiers() {
    QList<std::function<void (TagLib::APE::Tag*)> > modifiers;

    modifiers.append([](TagLib::APE::Tag* ape) { ape->setTitle("AaaaaaBbbbb"); });
    modifiers.append([](TagLib::APE::Tag* ape) { ape->setArtist("CcccccDddd"); });
    modifiers.append([](TagLib::APE::Tag* ape) { ape->setAlbum("EeeeeFfffff"); });
    modifiers.append([](TagLib::APE::Tag* ape) { ape->setYear(2097); });
    modifiers.append([](TagLib::APE::Tag* ape) { ape->setComment("Hello APE"); });

    return modifiers;
}

QList<std::function<void (TagLib::Ogg::XiphComment*)> > getXiphModifiers() {
    QList<std::function<void (TagLib::Ogg::XiphComment*)> > modifiers;

    modifiers.append([](TagLib::Ogg::XiphComment* xc) { xc->setTitle("KkkkkkLllll"); });
    modifiers.append([](TagLib::Ogg::XiphComment* xc) { xc->setArtist("MmmmNnnnnn"); });
    modifiers.append([](TagLib::Ogg::XiphComment* xc) { xc->setAlbum("OooooPppppp"); });
    modifiers.append([](TagLib::Ogg::XiphComment* xc) { xc->setYear(2096); });
    modifiers.append([](TagLib::Ogg::XiphComment* xc) { xc->setComment("Hello XIPH"); });

    return modifiers;
}

QList<std::function<void (TagLib::MPEG::File*)> > getMp3Modifiers() {
    QList<std::function<void (TagLib::MPEG::File*)> > modifiers;

    Q_FOREACH(auto id3v1Modifier, getId3v1Modifiers()) {
        modifiers.append(
            [id3v1Modifier](TagLib::MPEG::File* file) {
                id3v1Modifier(file->ID3v1Tag(true));
            }
        );
    }

    Q_FOREACH(auto id3v2Modifier, getId3v2Modifiers()) {
        modifiers.append(
            [id3v2Modifier](TagLib::MPEG::File* file) {
                id3v2Modifier(file->ID3v2Tag(true));
            }
        );
    }

    Q_FOREACH(auto apeModifier, getApeModifiers()) {
        modifiers.append(
            [apeModifier](TagLib::MPEG::File* file) {
                apeModifier(file->APETag(true));
            }
        );
    }

    return modifiers;
}

QList<std::function<void (TagLib::FLAC::File*)> > getFlacModifiers() {
    QList<std::function<void (TagLib::FLAC::File*)> > modifiers;

    Q_FOREACH(auto id3v1Modifier, getId3v1Modifiers()) {
        modifiers.append(
            [id3v1Modifier](TagLib::FLAC::File* file) {
                id3v1Modifier(file->ID3v1Tag(true));
            }
        );
    }

    Q_FOREACH(auto id3v2Modifier, getId3v2Modifiers()) {
        modifiers.append(
            [id3v2Modifier](TagLib::FLAC::File* file) {
                id3v2Modifier(file->ID3v2Tag(true));
            }
        );
    }

    Q_FOREACH(auto xiphModifier, getXiphModifiers()) {
        modifiers.append(
            [xiphModifier](TagLib::FLAC::File* file) {
                xiphModifier(file->xiphComment(true));
            }
        );
    }

    return modifiers;
}

QList<std::function<void (TagLib::File*)> > getModifiers(QString extension) {
    QList<std::function<void (TagLib::File*)> > modifiers;

    modifiers.append([](TagLib::File* file) { file->tag()->setTitle("Ooooooooo"); });
    modifiers.append([](TagLib::File* file) { file->tag()->setArtist("Aaaaaaaaaa"); });
    modifiers.append([](TagLib::File* file) { file->tag()->setAlbum("Eeeeeeeeee"); });
    modifiers.append([](TagLib::File* file) { file->tag()->setYear(2099); });
    modifiers.append([](TagLib::File* file) { file->tag()->setComment("No comment!"); });

    /* format-specific file modifications */

    if (extension == "mp3") {
        Q_FOREACH(auto mp3Modifier, getMp3Modifiers()) {
            modifiers.append(
                [mp3Modifier](TagLib::File* file) {
                    mp3Modifier(static_cast<TagLib::MPEG::File*>(file));
                }
            );
        }
    }
    else if (extension == "flac") {
        Q_FOREACH(auto flacModifier, getFlacModifiers()) {
            modifiers.append(
                [flacModifier](TagLib::File* file) {
                    flacModifier(static_cast<TagLib::FLAC::File*>(file));
                }
            );
        }
    }

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
            _err << "File extension not supported: " << _extension << Qt::endl;
            _error = true;
            return;
        }

        QFile file(filename);
        if (!file.open(QIODevice::ReadOnly)) {
            _err << "Could not open file: " << filename << Qt::endl;
            _error = true;
            return;
        }

        FileAnalyzer analyzer(fileInfo);
        analyzer.analyze();

        if (!analyzer.analysisDone()) {
            if (expectedResult == "invalid") return; /* OK */

            _err << "File analysis FAILED unexpectedly for " << filename << Qt::endl;
            _error = true;
            return;
        }

        _originalResult = getHashAsString(analyzer.hash());

        if (_originalResult != expectedResult) {
            _err << "Hash MISMATCH!" << Qt::endl
                 << "Filename: " << filename << Qt::endl
                 << "Expected: " << expectedResult << Qt::endl
                 << "Actual:   " << _originalResult << Qt::endl;
            _error = true;
            return;
        }

        QByteArray contents = file.readAll();
        _originalFileContents.setData(contents.data(), contents.length());
        _out << "Original data checksum: " << checksum(_originalFileContents) << Qt::endl;
    }

    bool error() const { return _error; }

    const TagLib::ByteVector& originalFileContents() const {
        return _originalFileContents;
    }

    const QString& extension() const { return _extension; }

    const QString& originalResult() const { return _originalResult; }

    bool testModifications(QList<std::function<void (TagLib::File*)> > modifiers) {
        QVector<TagLib::ByteVector> transformed;

        /* Single data transformations */
        auto singleTransformed = generateSingleModifiedData(modifiers);
        if (singleTransformed.size() == 0) return false; /* something went wrong */
        transformed << singleTransformed;

        /* Apply combinations of two consecutive modifications */
        auto multiTransformed = generateMultiModifiedData(modifiers);
        if (multiTransformed.size() == 0) return false; /* something went wrong */
        transformed << multiTransformed;

        unsigned correctHashCount = 0;
        Q_FOREACH(auto modifiedData, transformed) {
            FileAnalyzer analyzer(modifiedData, extension());
            analyzer.analyze();
            if (!analyzer.analysisDone()) {
                _err << "File analysis FAILED on modified data!" << Qt::endl;
                return false;
            }

            QString modifiedHash = getHashAsString(analyzer.hash());
            if (modifiedHash != _expectedResult) {
                _err << "Hash MISMATCH after modification!" << Qt::endl
                     << "Filename: " << _filename << Qt::endl
                     << "Expected: " << _expectedResult << Qt::endl
                     << "Original: " << originalResult() << Qt::endl
                     << "Modified: " << modifiedHash << Qt::endl;

                writeDebugFile(_filename + "_MODIFIED.data", modifiedData);
                return false;
            }

            correctHashCount++;
        }

        _out << "Got correct hash in " << correctHashCount << " of " << transformed.size()
             << " cases." << Qt::endl;

        return correctHashCount == (unsigned)transformed.size();
    }

private:

    QVector<TagLib::ByteVector> generateSingleModifiedData(
                                    QList<std::function<void (TagLib::File*)> > modifiers)
    {
        QVector<TagLib::ByteVector> transformed;
        transformed.reserve(modifiers.size());

        _out << "Generating single modifications" << Qt::endl;

        /* Apply each modification, and make sure they each generate a result different
           from the others and from the original data. */
        Q_FOREACH(auto modifier, modifiers) {
            auto modifiedData = applyModification(originalFileContents(), modifier);
            _out << "Modified data checksum: " << checksum(modifiedData) << Qt::endl;

            if (modifiedData.size() == 0) {
                _err << "Problem: modification went wrong, returned no result"
                     << Qt::endl;
                return QVector<TagLib::ByteVector>();
            }

            if (modifiedData == originalFileContents()) {
                _err << "Problem: modification ineffective; test would be unreliable"
                     << Qt::endl;
                return QVector<TagLib::ByteVector>();
            }

            if (transformed.contains(modifiedData)) {
                _err << "Problem: modification not unique; test would be unreliable"
                     << Qt::endl;
                return QVector<TagLib::ByteVector>();
            }

            transformed.append(modifiedData);
        }

        return transformed;
    }

    QVector<TagLib::ByteVector> generateMultiModifiedData(
                                    QList<std::function<void (TagLib::File*)> > modifiers)
    {
        QVector<TagLib::ByteVector> transformed;
        transformed.reserve(modifiers.size() * modifiers.size());

        _out << "Generating combined modifications" << Qt::endl;

        Q_FOREACH(auto modifier1, modifiers) {
            auto modifiedData1 = applyModification(originalFileContents(), modifier1);
            if (modifiedData1.size() == 0) {
                _err << "Problem: modification went wrong, returned no result"
                     << Qt::endl;
                return QVector<TagLib::ByteVector>();
            }

            Q_FOREACH(auto modifier2, modifiers) {
                auto modifiedData2 = applyModification(modifiedData1, modifier2);
                _out << "Modified data checksum: " << checksum(modifiedData2) << Qt::endl;
                if (modifiedData2.size() == 0) {
                    _err << "Problem: modification went wrong, returned no result"
                         << Qt::endl;
                    return QVector<TagLib::ByteVector>();
                }

                if (modifiedData2 == originalFileContents()) {
                    _err << "Problem: combined modification is no-op" << Qt::endl;
                    return QVector<TagLib::ByteVector>();
                }

                transformed.append(modifiedData2);
            }
        }

        return transformed;
    }

    void writeDebugFile(QString filename, const TagLib::ByteVector& contents) {
        QSaveFile file(filename);
        if (!file.open(QIODevice::WriteOnly)) {
            _err << "Could not open file for writing: " << filename << Qt::endl;
            return;
        }

        QDataStream stream(&file);
        stream.writeRawData(contents.data(), contents.size());

        file.commit();
    }

    TagLib::ByteVector applyModification(
            std::function<TagLib::ByteVector (const TagLib::ByteVector&)> modifier)
    {
        return modifier(originalFileContents());
    }

    TagLib::ByteVector applyModification(const TagLib::ByteVector& startData,
                                         std::function<void (TagLib::File*)> modifier)
    {
        /* create a stream from the original file contents (makes a copy) */
        TagLib::ByteVectorStream scratchStream(startData);

        TagLib::File* scratchFile = createFileObject(scratchStream, extension());

        if (scratchFile->readOnly() || !scratchFile->isOpen() || !scratchFile->isValid())
        {
            _err << "Problem when modifying tags: scratch file not modifyable"
                 << Qt::endl;
            return TagLib::ByteVector();
        }

        modifier(scratchFile);

        if (!scratchFile->save()) {
            _err << "Problem when saving modified scratch file to scratch stream"
                 << Qt::endl;
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
        err << "Exactly two non-empty arguments are required." << Qt::endl;
        return 2;
    }

    QString filename = arguments[1];
    QString expectedResult = arguments[2];

    FileTester tester(filename, expectedResult);
    if (tester.error()) return 1;

    bool result = tester.testModifications(getModifiers(tester.extension()));
    if (!result) return 1;

    out << "Success!" << Qt::endl;
    return 0;
}
