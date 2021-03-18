/*
    Copyright (C) 2014-2021, Kevin Andre <hyperquantum@gmail.com>

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
#include <QVersionNumber>

/* TagLib includes */
#include "flacfile.h"
#include "id3v2framefactory.h"
#include "mpegfile.h"
#include "taglib.h"
//#include "tbytevector.h"
#include "tbytevectorstream.h"

namespace PMP {

    FileAnalyzer::FileAnalyzer(const QString& filename)
     : FileAnalyzer(QFileInfo(filename))
    {
        logTagLibVersionOnce();
    }

    FileAnalyzer::FileAnalyzer(const QFileInfo& file)
     : _filePath(file.filePath()), _extension(getExtension(file.suffix())),
       _file(file.filePath()), _haveReadFile(false), _error(false), _analyzed(false)
    {
        _error |= _extension == Extension::None;

        logTagLibVersionOnce();
    }

    FileAnalyzer::FileAnalyzer(const QByteArray& fileContents,
                               const QString& fileExtension)
     : _extension(getExtension(fileExtension)),
       _fileContents(fileContents.data(), uint(fileContents.length())),
       _haveReadFile(true), _error(false), _analyzed(false)
    {
        _error |= _extension == Extension::None;

        logTagLibVersionOnce();
    }

    FileAnalyzer::FileAnalyzer(const TagLib::ByteVector& fileContents,
                               const QString& fileExtension)
     : _extension(getExtension(fileExtension)), _fileContents(fileContents),
       _haveReadFile(true), _error(false), _analyzed(false)
    {
        _error |= _extension == Extension::None;

        logTagLibVersionOnce();
    }

    bool FileAnalyzer::isFileSupported(QFileInfo& fileInfo,
                                       bool enableExperimentalFileFormats)
    {
        return fileInfo.isFile()
               && isExtensionSupported(fileInfo.suffix(), enableExperimentalFileFormats);
    }

    void FileAnalyzer::analyze()
    {
        if (_error || _analyzed) return;

        if (!_haveReadFile)
        {
            if (!_file.open(QIODevice::ReadOnly))
            {
                _error = true;
                return;
            }

            QByteArray contents = _file.readAll();
            _fileContents.setData(contents.data(), uint(contents.length()));
            _haveReadFile = true;
            if (_fileContents.isEmpty())
            {
                _error = true;
                return;
            }
        }

        switch (_extension)
        {
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

        if (!_error)
        {
            _analyzed = true;
        }
    }

    bool FileAnalyzer::hadError() const
    {
        return _error;
    }

    bool FileAnalyzer::analysisDone() const
    {
        return _analyzed;
    }

    FileHash FileAnalyzer::hash() const
    {
        return _hash;
    }

    FileHash FileAnalyzer::legacyHash() const
    {
        return _legacyHash;
    }

    AudioData const& FileAnalyzer::audioData() const
    {
        return _audio;
    }

    TagData const& FileAnalyzer::tagData() const
    {
        return _tags;
    }

    void FileAnalyzer::logTagLibVersionOnce()
    {
        static bool tagLibVersionLogged = false;

        if (tagLibVersionLogged)
            return;

        tagLibVersionLogged = true;

        QVersionNumber tagLibVersion { TAGLIB_MAJOR_VERSION, TAGLIB_MINOR_VERSION,
                                       TAGLIB_PATCH_VERSION };

        qDebug() << "compiled with TagLib version" << tagLibVersion;
    }

    bool FileAnalyzer::isExtensionSupported(QString const& extension,
                                            bool enableExperimentalFileFormats)
    {
        auto extensionEnum = getExtension(extension);

        (void)enableExperimentalFileFormats; /* no experimental formats at this time */

        ///* Experimental formats */
        //if (extensionEnum == Extension::SomeExperimentalFormat)
        //    return enableExperimentalFileFormats;

        /* all (other) recognized extensions are supported */
        return extensionEnum != Extension::None;
    }

    bool FileAnalyzer::preprocessFileForPlayback(QByteArray& fileContents,
                                                 QString extension)
    {
        /* only need to do something for MP3 files */
        if (getExtension(extension) != Extension::MP3) return true;

        /* strip the ID3v2 tag because DirectShow chokes on ID3v2.4 */

        TagLib::ByteVector scratch(fileContents.data(), uint(fileContents.length()));
        TagLib::ByteVectorStream stream(scratch);
        TagLib::MPEG::File tagFile(&stream, TagLib::ID3v2::FrameFactory::instance());
        if (!tagFile.isValid())
        {
            return false;
        }

        tagFile.strip(TagLib::MPEG::File::ID3v2); /* strip the ID3v2 */
        scratch = *stream.data(); /* get the stripped file contents */

        QByteArray strippedData(scratch.data(), int(scratch.size()));
        fileContents = strippedData;

        return true; /* success */
    }

    FileAnalyzer::Extension FileAnalyzer::getExtension(QString extension)
    {
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
        //QString albumArtist = TStringToQString(tag->)
        QString comment = TStringToQString(tag->comment());

        _tags = TagData(artist, title, album, comment);
    }

    FileHash FileAnalyzer::getHashFrom(TagLib::ByteVector const& data)
    {
        uint size = data.size();

        QCryptographicHash md5Hasher(QCryptographicHash::Md5);
        md5Hasher.addData(data.data(), int(size));

        QCryptographicHash sha1Hasher(QCryptographicHash::Sha1);
        sha1Hasher.addData(data.data(), int(size));

        return FileHash(size, sha1Hasher.result(), md5Hasher.result());
    }

    bool FileAnalyzer::stripID3v1(TagLib::ByteVector& data)
    {
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

    bool FileAnalyzer::stripAPE(TagLib::ByteVector& data)
    {
        // TODO FIXME : this does not work (correctly)

        auto length = data.size();
        if (length < 32U) return false;

        auto position = length - 32;
        if (data.mid(position, 8) != "APETAGEX")
        {
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
               || data[int(position)] != char(0xFFu)
               || data[int(position + 1)] == char(0xFFu)
               || (data[int(position + 1)] & char(0xE0u)) != char(0xE0u)))
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
            if (data[int(position)] == char(0xFFu)
                && data[int(position + 1)] != char(0xFFu)
                && (data[int(position + 1)] & char(0xE0u)) == char(0xE0u))
            {
                return true;
            }
        } while (position > 0);

        return false;
    }

    void FileAnalyzer::analyzeMp3()
    {
        TagLib::ByteVector scratch = _fileContents;
        TagLib::ByteVectorStream stream(scratch);

        TagLib::MPEG::File tagFile(&stream, TagLib::ID3v2::FrameFactory::instance());
        if (!tagFile.isValid())
        {
            _error = true;
            return;
        }

        _audio.setFormat(AudioData::MP3);

        getDataFromTag(tagFile.tag());

        auto* audioProperties = tagFile.audioProperties();
        if (audioProperties)
        {
            _audio.setTrackLengthMilliseconds(audioProperties->lengthInMilliseconds());
        }

        /* strip only ID3v2 */
        tagFile.strip(TagLib::MPEG::File::ID3v2);
        scratch = *stream.data(); /* get the stripped file contents */
        FileHash legacyHash = getHashFrom(scratch);

        bool finalDifferentFromLegacy = false;

        /* strip the rest (ID3v1 and APE) */
        finalDifferentFromLegacy |= stripID3v1(scratch);
        finalDifferentFromLegacy |= stripID3v1(scratch); /* ID3v1 might occur twice */
        finalDifferentFromLegacy |= stripAPE(scratch);

        /* get the final hash */
        if (finalDifferentFromLegacy)
        {
            _hash = getHashFrom(scratch);
            _legacyHash = legacyHash;
        }
        else
        {
            _hash = legacyHash;
            /* no 'legacy' hash */
        }
    }

    void FileAnalyzer::analyzeFlac()
    {
        TagLib::ByteVector scratch = _fileContents;
        TagLib::ByteVectorStream stream(scratch);

        TagLib::FLAC::File tagFile(&stream, TagLib::ID3v2::FrameFactory::instance());

        if (!tagFile.isValid())
        {
            _error = true;
            return;
        }

        _audio.setFormat(AudioData::FLAC);

        getDataFromTag(tagFile.tag());

        auto* audioProperties = tagFile.audioProperties();
        if (audioProperties)
        {
            _audio.setTrackLengthMilliseconds(audioProperties->lengthInMilliseconds());
        }

        /* strip all tags (hopefully) */
        tagFile.strip();
        tagFile.save(); /* apparently, strip() does not do a save by itself */
        scratch = *stream.data(); /* get the modifications into our scratch buffer */

        /* strip ID3v1 manually (TagLib might have failed to do this) */
        stripID3v1(scratch);
        stripID3v1(scratch); /* ID3v1 might occur twice */

        /* strip the header and metadata blocks */
        if (!stripFlacHeaders(scratch))
        {
            _error = true;
            return;
        }

        _hash = getHashFrom(scratch);
    }

    bool FileAnalyzer::stripFlacHeaders(TagLib::ByteVector& flacData)
    {
        const int metadataBlockHeaderSize = 4;

        if (flacData.size() < 4 /* fLaC */ + metadataBlockHeaderSize
                || !flacData.containsAt("fLaC", 0))
        {
            return false;
        }

        TagLib::ByteVectorStream stream(flacData);
        stream.seek(4); /* skip "fLaC" */

        while (true)
        {
            /* https://xiph.org/flac/format.html

               METADATA_BLOCK_HEADER:
                <1>  Last-metadata-block flag: '1' if this block is the last metadata
                      block before the audio blocks, '0' otherwise.
                <7>  BLOCK_TYPE
                <24> Length (in bytes) of metadata to follow (does not include the size of
                      the METADATA_BLOCK_HEADER)
            */
            TagLib::ByteVector blockHeader = stream.readBlock(metadataBlockHeaderSize);
            if (blockHeader.size() != metadataBlockHeaderSize) return false;

            bool lastBlockFlag = blockHeader[0] & '\x80';
            unsigned int blockSize = blockHeader.toUInt(1u, 3u);

            /* skip the current metadata block */
            stream.seek(long(blockSize), TagLib::ByteVectorStream::Position::Current);
            if (stream.tell() >= stream.length()) return false;

            if (lastBlockFlag) break;
        }

        auto audioDataStartPosition = stream.tell();
        stream.removeBlock(0, ulong(audioDataStartPosition));

        /* save modifications to parameter */
        flacData = *stream.data();
        return true;
    }

}
