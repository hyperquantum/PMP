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

#ifndef PMP_FILEANALYZER_H
#define PMP_FILEANALYZER_H

#include "audiodata.h"
#include "filehash.h"
#include "tagdata.h"

#include <QByteArray>
#include <QFile>
#include <QFileInfo>
#include <QString>

#include "tbytevector.h"

namespace TagLib
{
    class ByteVector;
    class ByteVectorStream;
    class Tag;
}

namespace PMP
{
    class FileAnalyzer {
    public:
        FileAnalyzer(const QString& filename);
        FileAnalyzer(const QFileInfo& file);
        FileAnalyzer(const QByteArray& fileContents,
                     const QString& fileExtension);
        FileAnalyzer(const TagLib::ByteVector& fileContents,
                     const QString& fileExtension);

        static bool isExtensionSupported(QString const& extension,
                                         bool enableExperimentalFileFormats = false);

        static bool preprocessFileForPlayback(QByteArray& fileContents,
                                              QString extension);

        void analyze();

        bool hadError() const;
        bool analysisDone() const;

        FileHash hash() const;
        FileHash legacyHash() const;

        AudioData const& audioData() const;
        TagData const& tagData() const;

    private:
        enum class Extension
        {
            None = 0,
            MP3, FLAC
        };

        static Extension getExtension(QString extension);
        static FileHash getHashFrom(TagLib::ByteVector const& data);

        void getDataFromTag(const TagLib::Tag* tag);

        static bool stripID3v1(TagLib::ByteVector& data);
        static bool stripAPE(TagLib::ByteVector& data);

        static bool findNextMpegFrame(TagLib::ByteVector const& data,
                                      unsigned long& position);
        static bool findPreviousMpegFrame(TagLib::ByteVector const& data,
                                          unsigned long& position);

        static bool stripFlacHeaders(TagLib::ByteVector& flacData);

        void analyzeMp3();
        void analyzeFlac();

        QString _filePath;
        Extension _extension;
        QFile _file;
        TagLib::ByteVector _fileContents;
        FileHash _hash;
        FileHash _legacyHash;
        AudioData _audio;
        TagData _tags;
        bool _haveReadFile;
        bool _error;
        bool _analyzed;
    };
}
#endif
