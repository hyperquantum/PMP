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

#ifndef PMP_FILEDATA_H
#define PMP_FILEDATA_H

#include "audiodata.h"
#include "filehash.h"
#include "tagdata.h"

namespace TagLib {
    class ByteVector;
    class ByteVectorStream;
    class Tag;
}

QT_FORWARD_DECLARE_CLASS(QFileInfo)

namespace PMP {

    class FileData {
    public:
        FileData(const FileHash& hash);

        FileData(const FileHash& hash,
            const QString& artist, const QString& title,
            const QString& album, const QString& comment,
            AudioData::FileFormat format, quint64 trackLengthMilliseconds);

        static bool supportsExtension(QString const& extension);

        static FileData analyzeFile(const QByteArray& fileContents,
                                    const QString& fileExtension);
        static FileData analyzeFile(const QString& filename);
        static FileData analyzeFile(QFileInfo& file);

        static FileData create(const FileHash& hash,
                               const QString& artist, const QString& title,
                               AudioData::FileFormat format = AudioData::UnknownFormat,
                               quint64 lengthMilliseconds = -1);

        bool isValid() const { return !_hash.empty(); }

        const FileHash& hash() const { return _hash; }

        const AudioData& audio() const { return _audio; }
        AudioData& audio() { return _audio; }

        const TagData& tags() const { return _tags; }
        TagData& tags() { return _tags; }

    private:

        static FileData analyzeMp3(TagLib::ByteVectorStream& fileContents);
        //static FileData const* analyzeWma(TagLib::ByteVectorStream& fileContents);

        static void getDataFromTag(const TagLib::Tag* tag,
                                   QString& artist, QString& title,
                                   QString& album, QString& comment);

        static FileHash getHashFrom(TagLib::ByteVector* data);

        FileHash _hash;
        AudioData _audio;
        TagData _tags;
    };
}
#endif
