/*
    Copyright (C) 2014-2016, Kevin Andre <hyperquantum@gmail.com>

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


#include "fileanalyzer.h"

#include <QCryptographicHash>

/* TagLib includes */
#include "id3v2framefactory.h"
#include "mpegfile.h"
//#include "tbytevector.h"
#include "tbytevectorstream.h"

namespace PMP {

    FileAnalyzer::FileAnalyzer(const QString& filename)
     : FileAnalyzer(QFileInfo(filename))
    {
        //
    }

    FileAnalyzer::FileAnalyzer(const QFileInfo& file)
     : _filePath(file.filePath()), _extension(getExtension(file.suffix())),
       _file(file.filePath()), _haveReadFile(false), _error(false), _analyzed(false)
    {
        _error |= _extension == Extension::None;
    }

    FileAnalyzer::FileAnalyzer(const QByteArray& fileContents,
                               const QString& fileExtension)
     : _extension(getExtension(fileExtension)),
       _fileContents(fileContents.data(), fileContents.length()),
       _haveReadFile(true), _error(false), _analyzed(false)
    {
        _error |= _extension == Extension::None;
    }

    FileAnalyzer::FileAnalyzer(const TagLib::ByteVector& fileContents,
                               const QString& fileExtension)
     : _extension(getExtension(fileExtension)), _fileContents(fileContents),
       _haveReadFile(true), _error(false), _analyzed(false)
    {
        _error |= _extension == Extension::None;
    }

    void FileAnalyzer::analyze() {
        if (_error || _analyzed) return;

        if (!_haveReadFile) {
            if (!_file.open(QIODevice::ReadOnly)) {
                _error = true;
                return;
            }

            QByteArray contents = _file.readAll();
            _fileContents.setData(contents.data(), contents.length());
            _haveReadFile = true;
            if (_fileContents.isEmpty()) {
                _error = true;
                return;
            }
        }

        switch (_extension) {
            case Extension::MP3:
                analyzeMp3();
                break;

            case Extension::FLAC:
                analyzeFlac();
                break;

            default:
            case Extension::None:
                _error = true; /* extension not recognized/supported */
                break;
        }

        if (!_error) {
            _analyzed = true;
        }
    }

    bool FileAnalyzer::hadError() const {
        return _error;
    }

    bool FileAnalyzer::analysisDone() const {
        return _analyzed;
    }

    FileHash FileAnalyzer::hash() const {
        return _hash;
    }

    FileHash FileAnalyzer::legacyHash() const {
        return _legacyHash;
    }

    AudioData const& FileAnalyzer::audioData() const {
        return _audio;
    }

    TagData const& FileAnalyzer::tagData() const {
        return _tags;
    }

    bool FileAnalyzer::isExtensionSupported(QString const& extension) {
        return getExtension(extension) != Extension::None;
    }

    FileAnalyzer::Extension FileAnalyzer::getExtension(QString extension) {
        auto lower = extension.toLower();
        if (lower == "mp3") return Extension::MP3;
        if (lower == "flac") return Extension::FLAC;

        return Extension::None;
    }

    void FileAnalyzer::getDataFromTag(const TagLib::Tag* tag)
    {
        if (!tag) { return; }

        QString artist = TStringToQString(tag->artist());
        QString title = TStringToQString(tag->title());
        QString album = TStringToQString(tag->album());
        QString comment = TStringToQString(tag->comment());

        _tags = TagData(artist, title, album, comment);
    }

    FileHash FileAnalyzer::getHashFrom(TagLib::ByteVector const& data) {
        uint size = data.size();

        QCryptographicHash md5Hasher(QCryptographicHash::Md5);
        md5Hasher.addData(data.data(), size);

        QCryptographicHash sha1Hasher(QCryptographicHash::Sha1);
        sha1Hasher.addData(data.data(), size);

        return FileHash(size, sha1Hasher.result(), md5Hasher.result());
    }

    bool FileAnalyzer::stripID3v1(TagLib::ByteVector& data) {
        auto length = data.size();
        if (length < 128U) return false;

        auto position = length - 128;
        if (data.at(position) != 'T'
            || data.at(position + 1) != 'A'
            || data.at(position + 2) != 'G')
        {
            return false; /* ID3v1 not found */
        }

        data = data.mid(0, position);
        return true;
    }

    bool FileAnalyzer::stripAPE(TagLib::ByteVector& data) {
        auto length = data.size();
        if (length < 32U) return false;

        auto position = length - 32;
        if (data.mid(position, 8) != "APETAGEX") {
            return false; /* APE not found */
        }

        data = data.mid(0, position);
        return true;
    }

    bool FileAnalyzer::findNextMpegFrame(TagLib::ByteVector const& data,
                                         unsigned long& position)
    {
        unsigned long size = data.size();
        if (size < 4) return false;
        size--; /* prevent out of bounds from checking the second byte */

        while (!(position >= size
               || data[position] != 0xFFu
               || data[position + 1] == 0xFFu
               || (data[position + 1] & 0xE0u) != 0xE0u))
        {
            position++;
        }

        return position < size;
    }

    bool FileAnalyzer::findPreviousMpegFrame(const TagLib::ByteVector& data,
                                             unsigned long& position)
    {
        unsigned long size = data.size();
        if (size < 4 || position == 0) return false;

        do {
            position--;
            if (data[position] == 0xFFu
                && data[position + 1] != 0xFFu
                && (data[position + 1] & 0xE0u) == 0xE0u)
            {
                return true;
            }
        } while (position > 0);

        return false;
    }

    void FileAnalyzer::analyzeMp3() {
        TagLib::ByteVector scratch = _fileContents;

        TagLib::ByteVectorStream stream(scratch);
        TagLib::MPEG::File tagFile(&stream, TagLib::ID3v2::FrameFactory::instance());
        if (!tagFile.isValid()) {
            _error = true;
            return;
        }

        _audio.setFormat(AudioData::MP3);

        getDataFromTag(tagFile.tag());

        TagLib::AudioProperties* audioProperties = tagFile.audioProperties();
        if (audioProperties) {
            _audio.setTrackLengthMilliseconds(audioProperties->lengthInMilliseconds());
        }

        /* strip only ID3v2 */
        tagFile.strip(TagLib::MPEG::File::ID3v2);
        scratch = *stream.data(); /* get the stripped file contents */
        FileHash legacyHash = getHashFrom(scratch);

        bool finalDifferentFromLegacy = false;

        /* strip the rest (ID3v1 and APE) */
        finalDifferentFromLegacy |= stripID3v1(scratch);
        finalDifferentFromLegacy |= stripAPE(scratch);

        /* get the final hash */
        if (finalDifferentFromLegacy) {
            _hash = getHashFrom(scratch);
            _legacyHash = legacyHash;
        }
        else {
            _hash = legacyHash;
            /* no 'legacy' hash */
        }
    }

    void FileAnalyzer::analyzeFlac() {
        _error = true; /* TODO: FLAC not yet supported*/
    }

}
